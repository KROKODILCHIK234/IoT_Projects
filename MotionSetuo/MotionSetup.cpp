#define DIR_LEFT 4
#define SPEED_LEFT 5

#define DIR_RIGHT 7
#define SPEED_RIGHT 6

#define FORWARD_LEFT LOW
#define BACKWARD_LEFT HIGH

#define FORWARD_RIGHT HIGH
#define BACKWARD_RIGHT LOW

void move(bool left_dir, int left_speed, bool right_dir, int right_speed) {
  digitalWrite(DIR_LEFT, left_dir);
  digitalWrite(DIR_RIGHT, right_dir);
  analogWrite(SPEED_LEFT, left_speed);
  analogWrite(SPEED_RIGHT, right_speed);
}

void forward(int speed) {
  move(FORWARD_LEFT, speed, FORWARD_RIGHT, speed);
}

void backward(int speed) {
  move(BACKWARD_LEFT, speed, BACKWARD_RIGHT, speed);
}

void turn_left(int steepness) {
  move(FORWARD_LEFT, 0, FORWARD_RIGHT, steepness);
}

void turn_right(int steepness) {
  move(FORWARD_LEFT, steepness, FORWARD_RIGHT, 0);
}

void rotate_left(int speed) {
  move(BACKWARD_LEFT, speed, FORWARD_RIGHT, speed);
}

void rotate_right(int speed) {
  move(FORWARD_LEFT, speed, BACKWARD_RIGHT, speed);
}

void setup() {
  pinMode(DIR_LEFT, OUTPUT);
  pinMode(DIR_RIGHT, OUTPUT);
  pinMode(SPEED_LEFT, OUTPUT);
  pinMode(SPEED_RIGHT, OUTPUT);
}

void loop() {

  forward(255);
  delay(2000);
  
  backward(255);
  delay(2000);
  
  turn_left(255);
  delay(2000);
  
  turn_right(255);
  delay(2000);
  
  rotate_left(255);
  delay(2000);
  
  rotate_right(255);
  delay(2000);
  
  move(FORWARD_LEFT, 0, FORWARD_RIGHT, 0); 
  delay(3000);
}
