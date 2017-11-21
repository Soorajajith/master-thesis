#define VERBOSE 0 //Setting this to 1 prints tons of data

//Network instructions
#define NETINSTR_STOP 0
#define NETINSTR_CONTACT 1
#define NETINSTR_DIR 2
#define NETINSTR_MOUSE 3
#define NETINSTR_BTNS 4
#define NETINSTR_PING 5
#define NETINSTR_IMGMODE 6
#define NETINSTR_SHUTDOWN 7

#define NETINSTR_ACK 'K' //Sent from the robot when it receives a NETINSTR_CONTACT

//Different buttons from the GUI
#define NETBTN_MAN 0
#define NETBTN_AUT 1
#define NETBTN_SENS_UP 2
#define NETBTN_SENS_DWN 3
#define NETBTN_SHUTDOWN 4

#define NET_HARD_TIMEOUT 3000 //ms
#define NET_SOFT_TIMEOUT NET_HARD_TIMEOUT/2 //ms

#define NET_CONTACT_WAIT 500 //ms

//Buffer lengths in bytes
#define BUFLEN 16
#define BUFLEN_IMG  57600 

#define UDP_PORT 8888

#define KEY_UP 0
#define KEY_DOWN 1
#define KEY_LEFT 2
#define KEY_RIGHT 3

#define DIR_FWD 0
#define DIR_BWD 1
#define DIR_L 2
#define DIR_R 3

//Network initialization and closing
int initNetwork(char* server);
void closeNetwork();
void setBlocking();
void setUnblocking();

//Message and instruction functions
byte createHeader(byte instr_type); //Returns a valid instruction header. Returns stop header if the type is invalid.
int getMessage(char* buf); //Returns numbers of bytes received, -1 when no data is available.
int getImage(char* buf); //Returns numbers of bytes received, -1 when no data is available.
int sendMessage(char* buf, int len); //Returns numbers of bytes sent, -1 on failure.
void sendPing(); //Sends a ping to the robot
void establishContact(); //Returns when contact is established with the robot.
void tryForContact(); //Tries once to establish contact with the robot.
bool isConnected();

void printBinary(char* buf, int len); //Prints the buffer as a string of 1's and 0's

//Client timer functions
int clientSoftTimeoutExceeded();
int clientHardTimeoutExceeded();
void resetClientTimer();

//Robot timer functions
int robotSoftTimeoutExceeded();
int robotHardTimeoutExceeded();
void resetRobotTimer();

//Generic timer functions
int timeoutExceeded(clock_t timer, int timeout);
void resetTimer(clock_t timer);

//Movement and button functions
void sendDigitalMovementToRobot(bool* keys);
void sendAnalogMovementToRobot(int x, int y);
void sendButton(int btn);

//Other functions
void sendShutdown(); //Shut downs the robot