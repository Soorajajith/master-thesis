#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include "robotnetwork.h"
#include "robotserial.h"

int sock, i, slen, recv_len;
struct sockaddr_in si_me, si_other;
struct timeval timer;

void startTimer(){
  gettimeofday(&timer, NULL);
}

void resetTimer(){
  gettimeofday(&timer, NULL);
}

long getTimerInMs(){
  struct timeval current;
  gettimeofday(&current, NULL);
  long ms = ((current.tv_sec - timer.tv_sec)*1000)+((current.tv_usec - timer.tv_usec)/1000);
  return ms;
}

int hardTimeoutExceeded(){
  return (getTimerInMs() >= NET_HARD_TIMEOUT);
}

int softTimeoutExceeded(){
  return (getTimerInMs() >= NET_SOFT_TIMEOUT);
}

//Prints the buffer in 1's and 0's
void printBinary(char* buf, int len){
  int i, j;
  for (i=0; i<len; i++){
    char byte = buf[i];
    for (j=7; j>=0; j--){
      int bit = (byte >> j)&1;
      if (bit == 1)
	printf("1");
      else
	printf("0");
    }
    printf(" ");
  }
  printf("\n");
}

void closeNetworkWithError(char* s){
  perror(s);
  closeNetwork();
}

//Checks that the correct amount of data bytes is received and that the data is correctly formatted
int isValidNetInstruction(char* instr, int instrlen){
  if (isValidNetHeader(instr[0])){
    int type = instr[0] >> 4; //The first four bits are the instruciton number, so a shift is made
    int len = instrlen-1; //header is included in len, so by subtracting 1 the number of data bytes is gotten
    switch (type){
    case NETINSTR_STOP:
    case NETINSTR_CONTACT:
    case NETINSTR_PING:
    case NETINSTR_SHUTDOWN:
      if (len == 0)
	return 1;
      break;
    case NETINSTR_BTNS:
      if (len == 1)
	return 1;
      break;
    case NETINSTR_IMGMODE:
      if (len == 1){
	if (instr[1] <= 3)
	  return 1;
      }
    case NETINSTR_DIR:
      if (len == 2)
	return 1;
      break;
    case NETINSTR_MOUSE:
      if (len == 2){
	//X should be between 1-160
	if (instr[1] == 0 || instr[1] > 160)
	  break;
	//Y should be between 1-120
	if (instr[2] == 0 || instr[2] > 120) //Y
	  break;
	return 1;
      }
      break;
    default:
      break;
    }
  }
  return 0;
}

int isValidNetHeader(char header){
  int type = header >> 4; //First four bits are the instruction type so a shift is made
  int data_bytes = header & 0x0F; //Last four bits are the number of bytes so an and is made
  switch (type){
  case NETINSTR_STOP:
  case NETINSTR_CONTACT:
  case NETINSTR_PING:
  case NETINSTR_SHUTDOWN:
    if (data_bytes == 0)
      return 1;
    break;
  case NETINSTR_BTNS:
  case NETINSTR_IMGMODE:
    if (data_bytes == 1)
      return 1;
    break;
  case NETINSTR_DIR:
  case NETINSTR_MOUSE:
    if (data_bytes == 2)
      return 1;
    break;
  default:
    break;
  }
}

void closeNetwork(){
  close(sock);
}

int initNetwork(){
  slen = sizeof(si_other);
  char buf[BUFLEN];

  //create a UDP socket
  if ((sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    closeNetworkWithError("UDP socket error");

  memset((char*) &si_me, 0, sizeof(si_me));

  si_me.sin_family = AF_INET;
  si_me.sin_port = htons(UDP_PORT);
  si_me.sin_addr.s_addr = htonl(INADDR_ANY);

  //bind socket to port
  if(bind(sock, (struct sockaddr*)&si_me, sizeof(si_me))==-1){
    closeNetworkWithError("UDP bind");
    return -1;
  }
}

int getMessage(char* buf){
  return recvfrom(sock,buf,BUFLEN,MSG_DONTWAIT,(struct sockaddr*)&si_other,&slen);
}

int sendMessage(char* buf, int len){
  return sendto(sock,buf,len,MSG_DONTWAIT,(struct sockaddr*)&si_other, slen);
}

void waitForContact(){
  char buf[512];
  int recv_len;

  setRGBLed(LED_R,LED_OFF,LED_OFF); //LED set to red

  printf("Connecting... ");
  fflush(stdout);
  while (1){
    //Loop constantly until a message is received
    if ((recv_len = getMessage(buf)) != -1){
      if (recv_len == 1){
    	if ((buf[0] >> 4) == NETINSTR_CONTACT){ //Message is a contact message, contact aquired
    	  sendMessage(NETINSTR_ACK,1);	  //Anwser message
	  setRGBLed(LED_OFF,LED_G,LED_OFF);//Set LED to green
	  printf("Connected.\n");
	  fflush(stdout);
	  return;
	}
      }
    }
  }
}
