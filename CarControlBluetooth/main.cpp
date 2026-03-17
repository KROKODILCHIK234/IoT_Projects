#include <SoftwareSerial.h>

#define DIR_RIGHT 4
#define SPEED_RIGHT 5

#define DIR_LEFT 7
#define SPEED_LEFT 6

#define LEFT_FORWARD HIGH
#define LEFT_BACKWARD LOW

#define RIGHT_FORWARD HIGH
#define RIGHT_BACKWARD LOW

SoftwareSerial btSerial(2, 3);

void move(bool left_dir, int left_speed, bool right_dir, int right_speed) {
  digitalWrite(DIR_LEFT, left_dir);
  digitalWrite(DIR_RIGHT, right_dir);
  analogWrite(SPEED_LEFT, left_speed);
  analogWrite(SPEED_RIGHT, right_speed);
}

void setup() {
  btSerial.begin(9600);

  pinMode(DIR_RIGHT, OUTPUT);
  pinMode(SPEED_RIGHT, OUTPUT);
  pinMode(DIR_LEFT, OUTPUT);
  pinMode(SPEED_LEFT, OUTPUT);
  
  move(LEFT_FORWARD, 0, RIGHT_FORWARD, 0);
}

void loop() {
  if (btSerial.available()) {
    char command = btSerial.read();

    switch (command) {
      case 'F': 
        move(LEFT_FORWARD, 150, RIGHT_FORWARD, 150); 
        break;
      case 'B': 
        move(LEFT_BACKWARD, 150, RIGHT_BACKWARD, 150); 
        break;
      case 'L': 
        move(LEFT_BACKWARD, 150, RIGHT_FORWARD, 150); 
        break;
      case 'R': 
        move(LEFT_FORWARD, 150, RIGHT_BACKWARD, 150); 
        break;
      case 'S': 
        move(LEFT_FORWARD, 0, RIGHT_FORWARD, 0); 
        break;
    }
  } 
}
