#include "/home/pi/robotbil/libs/instruction_types.h"
//This should be "../libs/instruction_types.h" but the Arduino
//IDE can't handle relative paths in that manner

//Motor in defines
#define RIGHT_FWD 9 
#define RIGHT_BWD 10 
#define LEFT_FWD 5 
#define LEFT_BWD 6

//LED pin defines
#define R_LED 3
#define G_LED 4
#define B_LED 2

byte header, instr_type, data_size;
byte response;
byte data[15];

int move_speed = 0;
int turn = 0;

byte LED[3] = {R_LED, G_LED, B_LED};

void setup(){
  for(int i=0; i<3; i++){
    pinMode(LED[i], OUTPUT);
    digitalWrite(LED[i], HIGH);
  }
  Serial.begin(19200);
}

void loop(){
  if (Serial.available()){
    if (get_instruction()){
      execute_instruction();
    }else{
      stop_robot();
    }
  }
}

void stop_robot(){
  move_speed = turn = 0;
  move_robot();
}

void move_robot(){
  int right = move_speed - turn;
  int left = move_speed + turn;
  
  //Right wheels
  if (right < 0){
    if (right < -255)
      right = -255;
    right = -right;
    digitalWrite(RIGHT_FWD, LOW);
    analogWrite(RIGHT_BWD, right);
  }else{
    if (right > 255)
      right = 255;
    digitalWrite(RIGHT_BWD, LOW);
    analogWrite(RIGHT_FWD, right);
  }
  
  //Left wheels
  if(left < 0){
    if (left < -255)
      left = -255;
    left = -left;
    digitalWrite(LEFT_FWD, LOW);
    analogWrite(LEFT_BWD, left);
  }else{
    if (left > 255)
      left = 255;
    digitalWrite(LEFT_BWD, LOW);
    analogWrite(LEFT_FWD, left);
  }
}

void execute_instruction(){
  switch(instr_type){
  case INSTR_STOP:
    stop_robot();
    return;
  case INSTR_FORWARD:
    if (move_speed < 0)
      move_speed = -move_speed;
    break;
  case INSTR_BACK:
    if (move_speed > 0)
      move_speed = -move_speed;
    break;
  case INSTR_ROT_R:
    move_speed = 0;
    turn = 255;
    break;
  case INSTR_ROT_L:
    move_speed = 0;
    turn = -255;
    break;
    
  //Instructions with one data byte. Range
  //is between 0-255 so the data is always valid.
  case INSTR_SPEED:
    if (move_speed < 0)
      move_speed = -data[0];
    else
      move_speed = data[0];
    break;
  case INSTR_RIGHT:
    turn = data[0];
    break;
  case INSTR_LEFT:
    turn = -data[0];
    break;
  case INSTR_LED:
    for (int i=0; i<3; i++){
      //bitRead(x,0) starts from the LSB, so since the order
      //is RGB the data is read "backwards"
      if(bitRead(data[0], 2-i))
        digitalWrite(LED[i], LOW);
      else
        digitalWrite(LED[i], HIGH);
    }
    
  //Other unknown instructions that aren't implemented.
  //This should never be called because of valid_header().
  default:
    stop_robot();
    return;
  }
  move_robot();
}

boolean get_instruction(){
  response = INSTR_OK;
  header = Serial.read();
  if (!valid_header()){
   //       Serial.print('a');
    response = INSTR_BAD_HEADER;
  }else{
    byte databytes_read = 0;
    delayMicroseconds(800); //Allow time for the next byte to arrive
    while(Serial.available()){
      data[databytes_read]=Serial.read();
      databytes_read++;
      delayMicroseconds(800); //Allow time for the next byte to arrive
    }
  
    if (databytes_read < data_size)
      response = INSTR_NOT_ENOUGH_DATA;
      
    else if (databytes_read > data_size)
      response = INSTR_TOO_MUCH_DATA;
      
    else if (!valid_data())
      response = INSTR_BAD_DATA;      
  }
  Serial.write(response);
  return (response == INSTR_OK);
}

boolean valid_data(){
  switch(instr_type){
  /*Instructions with no data bytes*/
  //Always valid because of this.
  case INSTR_STOP:
  case INSTR_FORWARD:
  case INSTR_BACK:
  case INSTR_ROT_R:
  case INSTR_ROT_L:
    return true;
    
  /*Instructions with one data byte*/
  
  //Range between 0-255 so the data is always valid.
  case INSTR_SPEED:
  case INSTR_RIGHT:
  case INSTR_LEFT:
    return true;
    
  //Last three bits relevant, so the value should be < 8
  case INSTR_LED:
    return data[0] < 8;
    
  //Other unknown instructions that aren't implemented.
  //This should never be called because of valid_header().
  default:
    return false;
  }
}

boolean valid_header(){
  instr_type = header >> 4;
  data_size = header & 15;

  
  switch(instr_type){
  //Instructions with no data bytes:
  case INSTR_STOP:
  case INSTR_FORWARD:
  case INSTR_BACK:
  case INSTR_ROT_R:
  case INSTR_ROT_L:
    if (data_size == 0)
      return true;
    break;
    
  //Instructions with one data byte:
  case INSTR_SPEED:
  case INSTR_RIGHT:
  case INSTR_LEFT:
  case INSTR_LED:
    if (data_size == 1)
      return true;
    break;
    
  //Other unknown instructions that aren't implemented
  default:
    break;
  }
  return false;
}
