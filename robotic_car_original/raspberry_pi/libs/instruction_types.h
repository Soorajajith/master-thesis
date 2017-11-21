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
