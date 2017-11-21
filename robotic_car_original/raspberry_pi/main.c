#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <termios.h>
#include <time.h>
#include <math.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <unistd.h>
#include <netinet/in.h>

#include "RaspiCamCV.h"
#include "libs/robotserial.h"
#include "libs/robotnetwork.h"
#include "imgmodes.h"

//Function declarations
RaspiCamCvCapture* cameraInit(int width, int height);

void getPixelValuesFromPos(IplImage* img, uint8_t* buffer, int x, int y);
void MyCallbackFunc(int event, int x, int y, int flags, void* param);
void stop();
void* UDPSend(void*arg);
void filterImages();
void executeInstruction();
void calculateMovement(int x, int area);
void quit();
void shutdownRobot();

int instructionReceived();

#define WIDTH 160
#define HEIGHT 120
#define FRAMERATE 30

#define X_TARGET WIDTH/2
#define X_MARGIN 5
#define RADIUS_TARGET 30
#define RADIUS_MARGIN 0
#define RADIUS_MIN 5

#define KP_RADIUS 20
#define KD_RADIUS 5

#define KP_TURN 7
#define KD_TURN 2

int watchdog_enabled = 1;
int video_enabled = 0;

IplImage* image;
IplImage* send_image;
IplImage* tresh_image;
IplImage* hsv_image;
uint8_t* pixelvals;
int hsv_range = 7;
int recv_len;

int send_ok = 1;
int udp_send_len = BUFLEN_IMG;
int udp_img_mode = 0;

int automatic_mode = 0;
int quitLoop = 0;

uint8_t serial_send_buffer[16];

int serial_fd;
uint8_t instruction[16];
uint8_t instruction_type;
int instruction_length;

int x_error_previous = 0;
int radius_error_previous = 0;


int main(int argc, char* argv[]){
  //Set flags from input arguments
  int i;
  for (i=0; i<argc;i++)
    printf("arg %d:%s\n",i,argv[i]);

  if (argc >= 2 && argv[1][0] == '1'){
    printf("Video enabled\n");
    video_enabled = 1;
  }
  if (argc >= 3 && argv[2][0] == '0')
    watchdog_enabled = 0;
    
  //Initialize network
  if (initNetwork() == -1){
    printf("Network socket failed to open\n");
  }else{
    printf("Network socket open on port %d\n",UDP_PORT);
  }
  fflush(stdout);

  //Open serial port
  printf("Opening serial port... ");
  serial_fd = openPort("/dev/ttyUSB0");
  if (serial_fd != -1){
    setInterfaceAttribs(serial_fd, B19200, 0);
    printf("Open.\n");
  }else{
    printf("Failed.\n");
  }

  RaspiCamCvCapture* capture = cameraInit(WIDTH,HEIGHT);

  //Init font
  CvFont font;
  double hScale=0.4;
  double vScale=0.4;
  int lineWidth=1;
  cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX|CV_FONT_ITALIC, hScale, vScale, 0, lineWidth, 8);

  //Initalize a default value for the pixel value
  pixelvals = (uint8_t*)malloc(3*sizeof(uint8_t));
  pixelvals[0]=50;
  pixelvals[1]=50;
  pixelvals[2]=50;

  if (video_enabled){
    cvNamedWindow("RaspiCamTest", 1);
    cvSetMouseCallback("RaspiCamTest", MyCallbackFunc, NULL);
  }

  //Establish contact with the client
  printf("Waiting for contact... ");
  fflush(stdout);
  waitForContact();
  printf("Done.\n");
  fflush(stdout);

  //Read an initial image so the thread has somethig to send
  image = raspiCamCvQueryFrame(capture);
  send_image = cvCloneImage(image);

  //Start the thread that sends the video feed
  pthread_t* udpthread = malloc(sizeof(udpthread));
  pthread_create(udpthread, NULL, &UDPSend, NULL);

  startTimer(); //Initalize the connection timer

  //Main loop starts
  do{
    //Read an image from the camera and reset the filtered image variables
    image = raspiCamCvQueryFrame(capture);
    hsv_image = cvCreateImage(cvGetSize(image),image->depth,image->nChannels);
    tresh_image = cvCreateImage(cvGetSize(image),image->depth,1);

    filterImages();

    if (instructionReceived()){
      executeInstruction();
      resetTimer();
    }else if (hardTimeoutExceeded() && watchdog_enabled){
      //Connection is assumed lost, reset robot and wait for contact again
      printf("Timeout exceeded!\n");
      send_ok = 0;
      stop();
      automatic_mode = 0;
      waitForContact();
      udp_img_mode = IMGMODE_BGR;
      send_ok = 1;
      resetTimer();
    }
     
    //Get moments
    CvMoments* moments = (CvMoments*)malloc(sizeof(CvMoments));
    cvMoments(tresh_image, moments, 1);
    double moment10 = cvGetSpatialMoment(moments, 1, 0);
    double moment01 = cvGetSpatialMoment(moments, 0, 1);

    //Get total area, center of mass and radius of assumed circle
    double area = cvGetCentralMoment(moments, 0, 0);
    int posx = (int)floor(moment10/area);
    int posy = (int)floor(moment01/area);
    int radius = (int)sqrt(area/M_PI); //Radius is in pixels

    //Draw circle on image
    cvCircle(image, cvPoint(posx,posy), radius, cvScalar(0,255,255,0), 1,8,0);

    if (automatic_mode && radius > RADIUS_MIN){
      calculateMovement(posx, radius);
    }

    //Copy image to be sent depending on what image mode is active
    switch(udp_img_mode){
    case IMGMODE_BGR:
      memcpy(send_image->imageData, image->imageData, BUFLEN_IMG);
      udp_send_len = BUFLEN_IMG;
      break;
    case IMGMODE_HSV:
      memcpy(send_image->imageData, hsv_image->imageData, BUFLEN_IMG);
      udp_send_len = BUFLEN_IMG;
      break;
    case IMGMODE_FILTERED:
      memcpy(send_image->imageData, tresh_image->imageData, BUFLEN_IMG/3);
      udp_send_len = BUFLEN_IMG/3;
      break;
    default:
      break;
    }

    if (video_enabled)
      cvShowImage("RaspiCamTest", image);

    char key = cvWaitKey(10);
    if (key == 27) //escape
      quitLoop = 1;
    else if (key == 82) //up
      hsv_range++;
    else if (key == 84) //down
      hsv_range--;

    //Release variables to avoid memory leaks
    cvReleaseImage(&hsv_image);
    cvReleaseImage(&tresh_image);
    free(moments);

  }while(!quitLoop);
  quit();
  return 0;
}

//Shuts down the robot completely by doing a halt command.
void shutdownRobot(){
  printf("Shutting down!\n");
  system("sudo halt");
  quitLoop = 1;
}

void quit(){
  stop();
  closeNetwork();
  printf("Network socket closed\n");
  setRGBLed(LED_OFF,LED_OFF,LED_OFF);
}

RaspiCamCvCapture* cameraInit(int width, int height){

  RASPIVID_CONFIG* config = (RASPIVID_CONFIG*)malloc(sizeof(RASPIVID_CONFIG));
  config->width=width;
  config->height=height;
  config->bitrate=0;
  config->framerate=0;
  config->monochrome=0;

  RaspiCamCvCapture* capture = (RaspiCamCvCapture*) raspiCamCvCreateCameraCapture2(0, config);
  free(config);
  return capture;
}

void getPixelValuesFromPos(IplImage* img, uint8_t* buffer, int x, int y){
  int i, channels;
  channels = img->nChannels;
  /*The image data element has its pixel in consecutive order, with the size of each
    pixel being the same amount of bytes that the number of channels in the picture.*/
  for (i=0; i < channels; i++){
    buffer[i]=CV_IMAGE_ELEM(img, uchar, y, (x*channels)+i); //CV_IMAGE_ELEM is a macro provided by te OpenCV library
  }
}

//This function is called upon double clicking in the video window
void MyCallbackFunc(int event, int x, int y, int flags, void* param){
  switch (event){
  case CV_EVENT_LBUTTONDBLCLK:
    if (hsv_image != NULL){
      getPixelValuesFromPos(hsv_image, pixelvals, y, x);
    }
  default:
    break;
  }
}

void stop(){
  char instr[1];
  formatInstruction(INSTR_STOP, NULL, instr);
  sendInstruction(instr);
}

void filterImages(){ 
  cvSmooth(image, hsv_image, CV_GAUSSIAN,3,0,0,0); //Smooth image
  cvCvtColor(hsv_image, hsv_image, CV_BGR2HSV); //Convert from BGR to HSV
  
  //Filters pixels in the image depending on whether they're in a certain range or not
  cvInRangeS(hsv_image, 
	     cvScalar(pixelvals[0]-hsv_range,100,50,0),
	     cvScalar(pixelvals[0]+hsv_range, 255, 255, 0),
	     tresh_image);
}

void calculateMovement(int x, int radius){
  char val;
  char buf[2];
  int x_error, radius_error;

  //Calculate the radius and position errors
  if (radius > RADIUS_TARGET-RADIUS_MARGIN && radius < RADIUS_TARGET+RADIUS_MARGIN)
    radius_error = 0;
  else
    radius_error = radius - RADIUS_TARGET;

  if (x > X_TARGET - X_MARGIN && x < X_TARGET+X_MARGIN)
    x_error = 0;
  else
    x_error = x - X_TARGET;  

  //Calculate the turn and move values, and set to 0 if within the deadzone
  int x_error_derivative = x_error - x_error_previous;
  int turn_value = KP_TURN*x_error + KD_TURN*x_error_derivative;

  if (turn_value > -150 && turn_value < 150)
    turn_value = 0;

  int radius_error_derivative = radius_error - radius_error_previous;
  int move_value = KP_RADIUS*radius_error + KD_RADIUS*radius_error_derivative;

  if (move_value > -150 && move_value < 150)
    move_value = 0;

  //Format the instructions and move the robot
  if (move_value < 0){
    formatInstruction(INSTR_FORWARD, NULL, buf);
    move_value = -move_value;
  }else{
    formatInstruction(INSTR_BACK, NULL, buf);
  }
  sendInstruction(buf);

  if (move_value > 255)
    move_value = 255;
  val = move_value;
  formatInstruction(INSTR_SPEED, &val, buf);
  sendInstruction(buf);

  if (move_value == 0)
    turn_value = 0;
  if (turn_value < 0){
    turn_value = -turn_value;
    if (turn_value > 255)
      turn_value = 255;
    val = turn_value;
    formatInstruction(INSTR_LEFT,&val,buf);
  }else{
    if(turn_value>255)
      turn_value=255;
    val = turn_value;
    formatInstruction(INSTR_RIGHT,&val,buf);
  }
  sendInstruction(buf);

  x_error_previous = x_error;
  radius_error_previous = radius_error;
}

int instructionReceived(){
  instruction_length = getMessage(instruction);
  if (instruction_length != -1 && isValidNetInstruction(instruction, instruction_length)){
    return 1;
  }
  return 0;
}

void setMoveDirection(char dir){
  char* instr;
  if (dir == DIR_FWD){
    formatInstruction(INSTR_FORWARD, NULL, instr);
  }
  else{
    formatInstruction(INSTR_BACK, NULL, instr);
  }

  sendInstruction(instr);
}

void setTurn(char dir, char speed){
  char instr[2];
  if (dir == TURN_R)
    formatInstruction(INSTR_RIGHT, &speed, instr);
  else
    formatInstruction(INSTR_LEFT, &speed, instr);

  sendInstruction(instr);  
}

void setMoveSpeed(uint8_t speed){
  char instr[2];
  formatInstruction(INSTR_SPEED, &speed, instr);
  sendInstruction(instr);
}

void executeInstruction(){
  //The first four bits are the instruciton type, so a right shift is made
  int type = instruction[0] >> 4;
  uint8_t move, turn;
  int turn_direction;
  switch(type){
  case NETINSTR_STOP:
    stop();
    break;
  case NETINSTR_CONTACT:
    sendMessage(NETINSTR_ACK,1);
    break;
  case NETINSTR_PING:
    //Do nothing, just acknowledge an instruction has been sent
    break;
  case NETINSTR_BTNS:
    switch(instruction[1]){
    case NETBTN_MAN:
      automatic_mode = 0;
      setRGBLed(LED_G,LED_OFF,LED_OFF); //Set RGB LED to green
      break;
    case NETBTN_AUT:
      automatic_mode = 1;
      setRGBLed(LED_B,LED_OFF,LED_OFF); //Set RGB LED to blue
      break;
    case NETBTN_SENS_UP:
      hsv_range++;
      break;
    case NETBTN_SENS_DWN:
      if (hsv_range > 0)
	hsv_range--;
      break;
    default:
      break;
    }
    break;
  case NETINSTR_DIR:
    if (!automatic_mode){
      //Set forward/backward instruction. Cast to int8_t since direction can be negative
      if ((int8_t)instruction[1] < 0){
	setMoveDirection(DIR_BWD);
	move = -2*(int8_t)instruction[1]-1;
      }else if ((int8_t)instruction[1] > 0){
	setMoveDirection(DIR_FWD);
	move = 2*(int8_t)instruction[1]+1;
      }else{
	move = 0;
	setMoveDirection(DIR_FWD);
      }
      
      //Set lefdt/right instruction. Cast to int8_t since direction can be negative
      if ((int8_t)instruction[2] < 0){
	turn_direction = TURN_L;
	turn = -2*(int8_t)instruction[2]-1;
      }else if ((int8_t)instruction[2] > 0){
	turn_direction = TURN_R;
	turn = 2*(int8_t)instruction[2]+1;
      }else{
	turn = 0;
	turn_direction = TURN_R;
      }

      //Send move and speed to motors
      setMoveSpeed(move);
      setTurn(turn_direction, turn);
    }
    break;
  case NETINSTR_MOUSE:
    //Fetch the pixel values if a converted image exists
    if (hsv_image != NULL){
      getPixelValuesFromPos(hsv_image, pixelvals, instruction[1], instruction[2]);
    }
    break;
  case NETINSTR_IMGMODE:
    //Change what image to send
    udp_img_mode = instruction[1];
    break;
  case NETINSTR_SHUTDOWN:
    shutdownRobot();
    break;
  default:
    break;
  }
  fflush(stdout);
}


//Thread to continuously send images through UDP
void* UDPSend(void *arg){
  clock_t start;
  int i,ms;
  while(1){
    ms=0;
    start = clock();
    if (send_ok)
      sendMessage(send_image->imageData, udp_send_len);
    while(ms < 1000/FRAMERATE)
      ms = (clock()-start)*1000/CLOCKS_PER_SEC;
  }
}
