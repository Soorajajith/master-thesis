#include <opencv/cv.h>

#define NETINSTR_STOP 0
#define NETINSTR_CONTACT 1
#define NETINSTR_DIR 2
#define NETINSTR_MOUSE 3
#define NETINSTR_BTNS 4
#define NETINSTR_PING 5
#define NETINSTR_IMGMODE 6
#define NETINSTR_SHUTDOWN 7

#define NETINSTR_ACK "K" //Should actually be 'K' but sendMessage complains

#define NETBTN_MAN 0
#define NETBTN_AUT 1
#define NETBTN_SENS_UP 2
#define NETBTN_SENS_DWN 3

#define UDP_PORT 8888

#define BUFLEN 16
#define BUFLEN_IMG 57600

#define NET_HARD_TIMEOUT 3000
#define NET_SOFT_TIMEOUT NET_HARD_TIMEOUT/2

int initNetwork();
int getMessage(char* buf);
int sendMessage(char* buf, int len);
int sendImage(IplImage* img);
void closeNetwork();

void waitForContact(); //Blocking until contact with client is established

void printBinary(char* buf, int len); //Prints the instruction in 1's and 0's

int isValidNetInstruction(char* instr, int instrlen);
int isValidNetHeader(char instr);

int hardTimeoutExceeded();
int softTimeoutExceeded();
void resetTimer();
void startTimer();
