#include <stdio.h>
#include <windows.h>
#include <time.h>
#include <Xinput.h>
#include <string>
#include <regex>
#include "resource.h"
#include "networkcomm.h"
#include "imgmodes.h"
#include "opencv2\core\core.hpp"
#include "opencv2\highgui\highgui.hpp"
#include "gui.h"
#include <conio.h>
#include <ctype.h>
#include <fstream>
#include <sstream>
#include <algorithm>

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "xinput.lib")
#pragma comment(lib, "advapi32")

using namespace cv;
using namespace std;

#define WINDOW_NAME "MindBot 1.1 - Prediction"
#define CMD_DISABLED 1

#define WIDTH 160
#define HEIGHT 120
#define DEPTH 8
#define CHANNELS_COLOR 3
#define CHANNELS_GRAY 1
#define FRAMERATE 30

void myCallbackFunc(int event, int x, int y, int flags, void* param);
void updateImgMode(int mode);
void buttonCallback(int state, void* userdata);
void drawGUI();

int img_mode = IMGMODE_BGR;
bool connecting = true;
bool automatic = false;
bool controller = false;
bool quit = false;
bool arrow_keys[] = {false, false, false, false};
byte keys[256];

char* server_ip_address = "192.168.1.19";
char* prediction_ip_address = "127.0.0.1";

int hsv_val = 7;

time_t timer;

IplImage* gui_bg;
IplImage* gui;
IplImage* img;

//Rectangles of interest for different GUI parts
CvRect roi_video = cvRect(VIDEO_XPOS, VIDEO_YPOS, WIDTH * 2, HEIGHT * 2);
CvRect roi_button_indicators = cvRect(BUTTON_RGB_XPOS, IMG_BUTTON_YPOS, BUTTON_TRACK_XPOS + IMG_BUTTON_WIDTH, BUTTON_HEIGHT+BUTTON_INDICATOR_HEIGHT+1);
CvRect roi_button_col1 = cvRect(BUTTON_COL_1_XPOS, 0, BUTTON_WIDTH, IMAGE_HEIGHT);
CvRect roi_status_indicators = cvRect(STATUS_XPOS, STATUS_YPOS, STATUS_SIDE+2, STATUS_SIDE*3);

HWND hwnd;

CvFont font;
double hscale = 1;
double vscale = 1;
int linewidth = 2;

int counter = 0;
int imageNr = 0; //Remember that everytime the program starts the images will start with this number
char filename[80];
ofstream instructionBytes;
bool predict = false;
bool predict_arrow_keys[] = { false, false, false, false };
char prediction[BUFLEN];
char exePath[MAX_PATH];
string exeFolderLocation;
STARTUPINFO si = { sizeof(STARTUPINFO) };
PROCESS_INFORMATION pi;

int main(void){
	printf("Enter new IP or press enter to keep default (192.168.1.19): ");
	char new_ip[17] = { 0 };
	while (1){
		fgets(new_ip, 16, stdin);
		if (new_ip[0] == '\n') //Only Enter pressed
			break;
		new_ip[strlen(new_ip) - 1] = NULL; //Replace the enter character with null
		//Regex for IP recognition found online at https://www.safaribooksonline.com/library/view/regular-expressions-cookbook/9780596802837/ch07s16.html
		if (regex_match(new_ip,
			regex("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?).){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$")
			)){
			//New valid IP has been entered
			server_ip_address = new_ip;
			break;
		}
		else{
			printf("Not a valid IP address! Enter new or press enter to keep default: ");
		}
	}


	//Hide console window if disabled
	if (CMD_DISABLED){
		HWND console = GetConsoleWindow();
		ShowWindow(console, SW_HIDE);
	}

	cvInitFont(&font, CV_FONT_VECTOR0, hscale, vscale, 0, linewidth, 8);

	//Initialize the image variables
	gui_bg = cvLoadImage("gui.png", 1); //Load GUI background
	gui = cvCloneImage(gui_bg); //Clone for future use and overwrites
	img = cvCreateImage(cvSize(WIDTH, HEIGHT), DEPTH, CHANNELS_COLOR); //Create empty image
	IplImage* img_filtered = cvCreateImage(cvSize(WIDTH, HEIGHT), DEPTH, CHANNELS_GRAY); //Create single channel image

	//Initialize image buffer and copy to the empty image. 
	//This avoids the first image shown being junk data.
	char imgData[BUFLEN_IMG] = { 0 };
	img->imageData = imgData;

	//Create window and set callback.
	cvNamedWindow(WINDOW_NAME);
	cvSetMouseCallback(WINDOW_NAME, &myCallbackFunc, NULL);

	//Fetch window handle and try to change icon (doesn't work at the moment)
	hwnd = (HWND)cvGetWindowHandle(WINDOW_NAME);
	HINSTANCE hinst = (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE);
	SetClassLong(hwnd,          // window handle 
		GCL_HICONSM,              // changes icon 
		(LONG)LoadIcon(hinst, MAKEINTRESOURCE(IDI_ICON1))
		);
	SetActiveWindow(hwnd);

	//Initalize gamepad variables
	XINPUT_STATE gamepad;
	int stick_x, stick_y;

	//Draw GUI and display initial image
	drawGUI();
	cvShowImage(WINDOW_NAME, gui);

	//Initalize network
	timer = clock();
	if (VERBOSE)
		printf("Starting network initalization...\n");
	initNetwork(server_ip_address);
	Sleep(NET_CONTACT_WAIT); //Wait for sockets to initalize
	if (VERBOSE)
		printf("Network initalized.\n");

	//Get the location of the executable file and start the prediction program automatically
	GetModuleFileName(NULL, exePath, sizeof(exePath));
	string::size_type pos = string(exePath).find_last_of("\\/");
	exeFolderLocation = string(exePath).substr(0, pos);
	exeFolderLocation += "\\prediction\\prediction.exe";
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	if (!CreateProcess(NULL, const_cast<LPSTR>(exeFolderLocation.c_str()), NULL, NULL, FALSE,
		0, NULL, NULL, &si, &pi)) {
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	//Just to be sure that the prediction program has started before setting up sockets
	Sleep(1000);

	//Init network to Python program
	connectPrediction(prediction_ip_address);
	Sleep(NET_CONTACT_WAIT); //Wait for sockets to initalize


	//Main loop starts here
	while (!quit){
		if (!IsWindow(hwnd)){
			break; //Exit the program if the main window is closed
		}

		if (isConnected()){				
			if (clientSoftTimeoutExceeded())
				sendPing(); //Send ping to let the robot know the connection is active

			//See if an image has been received
			int recvlen = getImage(imgData);
			if (recvlen != -1){
				//Image received, check length to see how many channels has been received.
				if (recvlen == BUFLEN_IMG){ //Full image received (three channels)
					memcpy(img->imageData, imgData, BUFLEN_IMG);
					sendPing();

					if (GetKeyState('S') < 0) { //Save images when 'S' key is held down
						//Save every 2nd image, unnecessary to save every image
						if (counter % 2 == 0) {
							sprintf_s(filename, "../Robotic_car/images/%u.png", imageNr);
							cvSaveImage(filename, img);
							
							//Save the instruction byte corresponding to the image saved
							instructionBytes.open("../Robotic_car/images/instruction_bytes.txt", std::ios_base::app);
							if (instructionBytes) {
								instructionBytes << filename << " " << getInstructionByte() << '\n';
							}
							instructionBytes.close();

							imageNr++;
						}
						counter++;
					}

					if (GetKeyState('P') & 1) { //Start running based on predictions from Network when 'P' is toggled on.
						predict = true;
						sendDigitalMovementToRobot(predict_arrow_keys);
						//Send every third image to the network
						if (counter % 3 == 0) {
							if (sendImageToNetwork(img) == SOCKET_ERROR) {
								printf("sendImagePy failed with error code : %d\n", WSAGetLastError());
							}
							if (getPrediction(prediction) == SOCKET_ERROR) {
								sendPing();
								int error = WSAGetLastError();
								if (error != WSAEWOULDBLOCK){ 
									if (error != WSAEMSGSIZE){ 
										printf("tryForContact: recvfrom failed with error code : %d\n", error);
									}
								}
								else {
									printf("%d\n", error);
								}
							}
							else {
								//Set the instuctions to be sent to the robot according the prediction from the network
								memset((void *)predict_arrow_keys, 0, sizeof(predict_arrow_keys));
								if (prediction[0] == 2) {
									predict_arrow_keys[KEY_UP] = true;
								}
								else if (prediction[0] == 1) {
									predict_arrow_keys[KEY_LEFT] = true;
								}
								else if (prediction[0] == 0) {
									predict_arrow_keys[KEY_RIGHT] = true;
								}
								else if (prediction[0] == 3) {
									predict_arrow_keys[KEY_UP] = true;
									predict_arrow_keys[KEY_RIGHT] = true;
								}
								else if (prediction[0] == 4) {
									predict_arrow_keys[KEY_UP] = true;
									predict_arrow_keys[KEY_LEFT] = true;
								}
							}
							sendPing();
						}
						counter++;
						if (counter == 3) {
							counter = 0;
						}
					}
					else {
						if (predict) {
							predict = false;
							memset((void *)predict_arrow_keys, 0, sizeof(predict_arrow_keys));
						}
						
					}
					
				}
				else if (recvlen == BUFLEN_IMG / 3){ // One channel received 
					//Copy to img_filtered temporary and convert it to three channels
					memcpy(img_filtered->imageData, imgData, BUFLEN_IMG / 3);
					cvCvtColor(img_filtered, img, CV_GRAY2BGR);
				}
				else{
					//Do nothing.
				}
			}
		}
		else if (connecting){
			if (VERBOSE)
				printf("Trying for contact\n");
			//Send a connect probe to the robot. If a response is read isConnected() will return true.
			tryForContact();
			if (isConnected()){					
				//Reset the program to "default" state
				updateImgMode(IMGMODE_BGR);
				automatic = false;

				//Stop trying to connect
				connecting = false;
			}
		}

		//Try to read controller
		if (XInputGetState(0, &gamepad)==ERROR_SUCCESS){ //Controller is connected
			//Enable controller and read joystick state
			controller = true;
			stick_x = gamepad.Gamepad.sThumbLX / 256;
			stick_y = gamepad.Gamepad.sThumbLY / 256;
		}
		else{
			controller = false;
		}

		drawGUI();

		//Update the video and button areas of the image
		cvSetImageROI(gui, roi_video);
		if (isConnected() && !robotHardTimeoutExceeded()){
			cvResize(img, gui);

		}
		else{
			cvSetImageROI(gui_bg, roi_video);
			cvCopy(gui_bg, gui, NULL);
			cvResetImageROI(gui_bg);
		}
		cvResetImageROI(gui);

		cvShowImage(WINDOW_NAME, gui);
			
		cvWaitKey(1); //Important for the image to update properly

		GetKeyboardState(keys); //Read keyboard state


		//Check which keys are held down. Several keys can be held at
		//the same time so several if cases are used.
		if (keys[VK_ESCAPE] > 1){
			break;
		}
		if (keys[0x52] > 1){ //'R' key
			connecting = true;
		}
		if (keys[0x31] > 1){ //'1' key
			updateImgMode(IMGMODE_BGR);
		}
		if (keys[0x32] > 1){ //'2' key
			updateImgMode(IMGMODE_HSV);
		}
		if (keys[0x33] > 1){ //'3' key
			updateImgMode(IMGMODE_FILTERED);
		}

		if (keys[VK_UP] > 1){
			arrow_keys[KEY_UP] = true;
		}
		else{
			arrow_keys[KEY_UP] = false;
		}

		if (keys[VK_DOWN] > 1){
			arrow_keys[KEY_DOWN] = true;
		}
		else{
			arrow_keys[KEY_DOWN] = false;
		}

		if (keys[VK_LEFT] > 1){
			arrow_keys[KEY_LEFT] = true;
		}
		else{
			arrow_keys[KEY_LEFT] = false;
		}

		if (keys[VK_RIGHT] > 1){
			arrow_keys[KEY_RIGHT] = true;
		}
		else{
			arrow_keys[KEY_RIGHT] = false;
		}

		//Send movement to robot depending on active input mode and control mode
		if (!automatic){
			if (controller)
				sendAnalogMovementToRobot(stick_x, stick_y);
			else if (!predict)
				sendDigitalMovementToRobot(arrow_keys);
		}
	}

	//On exit close the sockets, prediction process and exit the program.
	closeNetwork();
	closeNetworkPrediction();
	DWORD exitCode = NULL;
	TerminateProcess(pi.hProcess, GetExitCodeProcess(pi.hProcess, &exitCode));
	if (exitCode == 0) {
		printf("TerminateProcess failed: %d", GetLastError());
	}
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return 0;
}

//Copies an area from the original background image to the active GUI image
void copyAreaOnGUI(CvRect roi){
	cvSetImageROI(gui, roi);
	cvSetImageROI(gui_bg, roi);
	cvCopy(gui_bg, gui);
	cvResetImageROI(gui_bg);
	cvResetImageROI(gui);
}

void drawGUI(){
	//Reset button areas
	copyAreaOnGUI(roi_button_indicators);
	copyAreaOnGUI(roi_button_col1);
	copyAreaOnGUI(roi_status_indicators);

	//Draw indicator for image mode
	int xpos;
	switch (img_mode)
	{
	case IMGMODE_BGR:
		xpos = BUTTON_RGB_XPOS;
		break;
	case IMGMODE_HSV:
		xpos = BUTTON_HSV_XPOS;
		break;
	default:
		xpos = BUTTON_TRACK_XPOS;
		break;
	}

	cvDrawRect(gui,
		cvPoint(xpos, IMG_BUTTON_INDICATOR_YPOS),
		cvPoint(xpos + IMG_BUTTON_WIDTH, IMG_BUTTON_INDICATOR_YPOS + BUTTON_INDICATOR_HEIGHT),
		cvScalar(26, 125, 191),
		CV_FILLED);

	//Draw indicator for man/aut
	int ypos;
	if (automatic){
		ypos = BUTTON_AUT_YPOS;
	}
	else{
		ypos = BUTTON_MAN_YPOS;
	}

	cvDrawRect(gui,
		cvPoint(BUTTON_COL_1_XPOS, ypos + BUTTON_HEIGHT),
		cvPoint(BUTTON_COL_1_XPOS + BUTTON_WIDTH-1, ypos + BUTTON_HEIGHT + BUTTON_INDICATOR_HEIGHT),
		cvScalar(26, 125, 191),
		CV_FILLED);

	//Draw status indicator
	if (isConnected()){
		//draw green rect
		cvDrawRect(gui,
			cvPoint(STATUS_XPOS, STATUS_YPOS),
			cvPoint(STATUS_XPOS + STATUS_SIDE, STATUS_YPOS + STATUS_SIDE - 1),
			cvScalar(0, 255, 0),CV_FILLED);
	}
	else if (connecting){
		//draw yellow rect
		cvDrawRect(gui,
			cvPoint(STATUS_XPOS, STATUS_YPOS),
			cvPoint(STATUS_XPOS + STATUS_SIDE, STATUS_YPOS + STATUS_SIDE - 1),
			cvScalar(0, 255, 255), 
			CV_FILLED);
	}

	//Draw controller indicator
	if (controller){
		cvDrawRect(gui,
			cvPoint(STATUS_XPOS, CONTROLLER_YPOS),
			cvPoint(STATUS_XPOS + STATUS_SIDE, CONTROLLER_YPOS + STATUS_SIDE - 1),
			cvScalar(0, 255, 0), CV_FILLED);
	}
}

void updateImgMode(int mode){
	if (img_mode != mode){
		img_mode = mode; //Set internal mode
		char buf[2]; //Create buffer to be sent
		buf[0] = createHeader(NETINSTR_IMGMODE); //Create header
		buf[1] = mode; //Set data byte to the image mode
		sendMessage(buf, 2); //Send buffer to robot
	}
}

//Mouse click callback function
void myCallbackFunc(int event, int x, int y, int flags, void* param){
	switch (event)
	{
	//On double click check if the click is inside the video area
	case CV_EVENT_LBUTTONDBLCLK:
		//Adjust x and y for the video position in the window
		x -= VIDEO_XPOS;
		y -= VIDEO_YPOS;

		//The video is upscaled by 2x so x and y are halved to compensate
		x /= 2;
		y /= 2;

		//Check if x and y are within the video area
		if (x > 0 && x < WIDTH && y > 0 && y < HEIGHT){
			char buf[3]; //Create buffer to be sent
			buf[0] = createHeader(NETINSTR_MOUSE);//Create header
			buf[1] = x; //Set data byte 1
			buf[2] = y; //Set data byte 2
			sendMessage(buf, 3);
		}
		break;
	case CV_EVENT_LBUTTONDOWN:
	//case CV_EVENT_LBUTTONUP:
		//Image button area
		if (y >= IMG_BUTTON_YPOS && y <= IMG_BUTTON_YPOS + BUTTON_HEIGHT+BUTTON_INDICATOR_HEIGHT){
			//possibility for button press
			if (x >= BUTTON_RGB_XPOS && x <= BUTTON_RGB_XPOS + IMG_BUTTON_WIDTH){
				updateImgMode(IMGMODE_BGR);
			}
			else if (x >= BUTTON_HSV_XPOS && x <= BUTTON_HSV_XPOS + IMG_BUTTON_WIDTH){
				updateImgMode(IMGMODE_HSV);
			}
			else if (x >= BUTTON_TRACK_XPOS && x <= BUTTON_TRACK_XPOS + IMG_BUTTON_WIDTH){
				updateImgMode(IMGMODE_FILTERED);
			}
		}
		//Button column 1
		if (x >= BUTTON_COL_1_XPOS && x <= BUTTON_COL_1_XPOS + BUTTON_WIDTH){
			if (y >= BUTTON_MAN_YPOS && y <= BUTTON_MAN_YPOS + BUTTON_HEIGHT + BUTTON_INDICATOR_HEIGHT){
				printf("Manual control\n");
				automatic = false;
				sendButton(NETBTN_MAN);
			}
			else if (y >= BUTTON_AUT_YPOS && y <= BUTTON_AUT_YPOS + BUTTON_HEIGHT + BUTTON_INDICATOR_HEIGHT){
				printf("Automatic control\n");
				automatic = true;
				sendButton(NETBTN_AUT);
			}
			else if (y >= SENSITIVITY_YPOS && y <= SENSITIVITY_YPOS+SENSITIVITY_BTN_HEIGHT){
				if (x >= BUTTON_COL_1_XPOS && x <= BUTTON_COL_1_XPOS + SENSITIVITY_BTN_WIDTH){
					printf("Sensitivity up\n");
					sendButton(NETBTN_SENS_UP);
				}
				else if (x >= SENSITIVITY_DWN_BTN_XPOS && x <= SENSITIVITY_DWN_BTN_XPOS + SENSITIVITY_BTN_WIDTH){
					printf("Sensitivity down\n");
					sendButton(NETBTN_SENS_DWN);
				}
			}
		}
		//Button column 2
		if (x >= BUTTON_COL_2_XPOS && x <= BUTTON_COL_2_XPOS + BUTTON_WIDTH){
			if (y >= BUTTON_RECONNECT_YPOS && y <= BUTTON_RECONNECT_YPOS + BUTTON_HEIGHT){
				printf("Reconnect\n");
				connecting = true;
			}
			if (y >= BUTTON_EXIT_YPOS && y <= BUTTON_EXIT_YPOS + BUTTON_HEIGHT){
				printf("Quit\n");
				quit = true;
			}
			if (y >= BUTTON_SHUTDOWN_YPOS && y <= BUTTON_SHUTDOWN_YPOS + BUTTON_HEIGHT){
				int msgBoxId = MessageBox(
					NULL,
					"Do you want to shut down the robot\nand exit the program?",
					"Shutdown Warning",
					MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON2);

				if (msgBoxId == IDOK){
					sendShutdown();
					quit = true;
				}
			}
		}
		break;
	default:
		break;
	}
}