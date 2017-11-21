#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <stdint.h>
#include "robotserial.h"

int port_fd = -1;

void setRGBLed(uint8_t val1, uint8_t val2, uint8_t val3){
  uint8_t command[2];
  uint8_t data = val1 | val2 | val3;
  formatInstruction(INSTR_LED, &data, command);
  uint8_t ans = sendInstruction(command);
  if (ans != INSTR_OK)
    printf("LED instruction not OK\n");
}

int getNumberOfDataBytes(int instruction_type)
{
  switch(instruction_type)
    {
      //Instructions with no data bytes
    case INSTR_STOP:
    case INSTR_FORWARD:
    case INSTR_BACK:
    case INSTR_ROT_R:
    case INSTR_ROT_L:
      return 0;
      
      //Instructions with one data byte
    case INSTR_SPEED:
    case INSTR_RIGHT:
    case INSTR_LEFT:
    case INSTR_LED:
      return 1;
      
      //Other unknown instructions
    default:
      return -1;
  }
}

int setInterfaceAttribs (int fd, int speed, int parity){
  //All code (including comments) found online
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  if (tcgetattr (fd, &tty) != 0)
    {
      printf("error %d from tcgetattr", errno);
      return -1;
    }
  
  cfsetospeed (&tty, speed);
  cfsetispeed (&tty, speed);

  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
  // disable IGNBRK for mismatched speed tests; otherwise receive break
  // as \000 chars
  tty.c_iflag &= ~IGNBRK;         // disable break processing
  tty.c_lflag = 0;                // no signaling chars, no echo,
  // no canonical processing
  tty.c_oflag = 0;                // no remapping, no delays
  tty.c_cc[VMIN]  = 0;            // read doesn't block
  tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
  
  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
  
  tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
  // enable reading
  tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
  tty.c_cflag |= parity;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;
  
  if (tcsetattr (fd, TCSANOW, &tty) != 0)
    {
      printf ("error %d from tcsetattr", errno);
      return -1;
    }
  return 0;
}

int openPort(char* portname)
{
  //Code, including comments, found online
  int fd; /* File descriptor for the port */
  fd = open(portname, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd == -1)
  {
    //Could not open the port.
    char error_message[75] = "open_port: Unable to open ";
    strcat(error_message, portname);
    perror(error_message);
  }
  else
    fcntl(fd, F_SETFL, 0);
  port_fd = fd;
  sleep(2);
  return (fd);
}

uint8_t sendInstruction(uint8_t* instr){
  if (port_fd == -1){
    //Port not open
    printf("send_command: Error: port not open\n");
    return -1;
  }else{
    tcflush(port_fd, TCIOFLUSH); //Flush buffer just to be safe

    //Use a bit mask to get the number of data bytes,
    //and add 1 for the header
    int length = (instr[0] & 15) + 1;

    //Try to write, signal error otherwise
    if (write(port_fd, instr, length)<=0)
      printf("Error writing to buffer\n");

    //Sleep to give the arduino time to respond
    usleep((length + 25) * 100);
    uint8_t response[10];
        memset(&response, '\0', sizeof response);

    read(port_fd, &response, 1);
    return response[0];
  }
}

/*
A command consists of 1-16 bytes; one header and 0-15 data bytes. The first four bits
of the header describes the instruction type, and the last for how many data bytes
that follows.
*/

int formatInstruction(int instruction_type, uint8_t* data, uint8_t* instruction_buffer){
  //Check if the instrucion type is one of the available ones. Since the types start at 0 and
  //increment by 1, the upper bound is NUMBER_OF_INSTRUCTIONS.
  if (instruction_type < 0 || instruction_type > NUMBER_OF_INSTRUCTIONS)
    return -1;

 uint8_t header = instruction_type << 4; //Shift the instruction type left to the correct bits

 int data_length = getNumberOfDataBytes(instruction_type);
 
 if (data_length == -1)
   return -1; //something wrong with the instruction

 if (data_length != 0 && data == NULL)
   return -1; //something wrong with the data

 header += data_length; //Add the number of data bytes to the header

 instruction_buffer[0]=header; //Add the header to the buffer
 
 //The data is added to the buffer. Currently there is no need to check
 //the data since all values are accepted, but this might change in later implementations.
 int i;
 for (i=1; i<=data_length; i++)
 instruction_buffer[i] = data[i-1]; //Add the data to the buffer
  
  return 1;
}

//Prints the binary value of an instruction to the terminal.
void printBinaryInstruction(uint8_t* instr){
  int size = (instr[0] & 15)+1;
  int i, j;
  int bit;
  char byte[8];
  for (i=0; i<size; i++){
    for (j=0; j<8; j++){
      bit = (instr[i] >> j) & 1;
      if (bit == 1)
	byte[7-j]='1';
      else
	byte[7-j]='0';
    }
    printf("%s ",byte);
  }
  printf("\n");
}

//Prints the binary value of an answer to the terminal.
void printBinaryAnswer(uint8_t answer){
  char byte[8];
  int bit;
  int i;
  for (i=0; i<8; i++){
    bit = (answer >> 1) & 1;
    if (bit == 1)
      byte[7-i]='1';
    else
      byte[7-i]='0';
  }
  printf("%s\n",byte);
}
