//Instruction types
#define INSTR_STOP    0
#define INSTR_SPEED   1
#define INSTR_FORWARD 2
#define INSTR_BACK    3
#define INSTR_RIGHT   4
#define INSTR_LEFT    5
#define INSTR_ROT_R   6
#define INSTR_ROT_L   7
#define INSTR_LED     9

//Increase this number if more are added
#define NUMBER_OF_INSTRUCTIONS 10

//Response types
#define INSTR_OK 0
#define INSTR_BAD_HEADER 1
#define INSTR_BAD_DATA 2
#define INSTR_TOO_MUCH_DATA 3
#define INSTR_NOT_ENOUGH_DATA 4

#define LED_R 4
#define LED_G 2
#define LED_B 1
#define LED_OFF 0

#define DIR_FWD 0
#define DIR_BWD 1

#define TURN_R 2
#define TURN_L 3


//Serial port functions
int setInterfaceAttribs(int fd, int speed, int parity);
int openPort(char* portname);

//Instruction functions
int formatInstruction(int instruction_type, uint8_t* data, uint8_t* instruction_buffer); //Creates an instruction in the instruction buffer
int getNumberOfDataBytes(int instruction_type); //Returns the number of data bytes for the given type, -1 if the instruction is not defined.
uint8_t sendInstruction(uint8_t* instr); //Sends a command to the Arduino. Returns the Arduino's answer.

//Print functions
void printBinaryInstruction(uint8_t* instr); //Displays command in binary
void printBinaryAnswer(uint8_t answer); //Displays the Arduino answer in binary

//Sets the three colors of the RGB led. The arguments can be LED_R, LED_G, LED_B or LED_OFF. Their order is not relevant.
void setRGBLed(uint8_t val1, uint8_t val2, uint8_t val3);
