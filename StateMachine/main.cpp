#define DIR_RIGHT 4
#define SPEED_RIGHT 5
#define DIR_LEFT 7
#define SPEED_LEFT 6

#define LEFT_FORWARD LOW
#define LEFT_BACKWARD HIGH
#define RIGHT_FORWARD LOW
#define RIGHT_BACKWARD HIGH

#define TRIG_FRONT 8
#define ECHO_FRONT 9
#define TRIG_LEFT 10
#define ECHO_LEFT 11

enum State {
  WALL_FOLLOW,
  TURN_RIGHT,
  OFFSET_FORWARD,
  TURN_LEFT
};

State currentState = WALL_FOLLOW;

#define TARGET_DIST_LEFT 15
#define STOP_DISTANCE_FRONT 20
#define LEFT_LOST_DIST 35

#define BASE_SPEED 150
#define TURN_SPEED 150

const unsigned long TURN_90_TIME = 800; 
const unsigned long OFFSET_TIME = 500;  

unsigned long stateStartTime = 0;
float Kp = 4.0; 

void move(bool left_dir, int left_speed, bool right_dir, int right_speed) {
  digitalWrite(DIR_LEFT, left_dir);
  digitalWrite(DIR_RIGHT, right_dir);
  analogWrite(SPEED_LEFT, constrain(left_speed, 0, 255));
  analogWrite(SPEED_RIGHT, constrain(right_speed, 0, 255));
}

int getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000); 
  if (duration == 0) return 999; 
  return duration / 58;
}

void setup() {
  Serial.begin(9600);
  pinMode(DIR_RIGHT, OUTPUT);
  pinMode(SPEED_RIGHT, OUTPUT);
  pinMode(DIR_LEFT, OUTPUT);
  pinMode(SPEED_LEFT, OUTPUT);
  pinMode(TRIG_FRONT, OUTPUT);
  pinMode(ECHO_FRONT, INPUT);
  pinMode(TRIG_LEFT, OUTPUT);
  pinMode(ECHO_LEFT, INPUT);
  delay(3000);
}

void loop() {
  int distFront = getDistance(TRIG_FRONT, ECHO_FRONT);
  int distLeft = getDistance(TRIG_LEFT, ECHO_LEFT);

  switch (currentState) {
    case WALL_FOLLOW:
      if (distFront > 0 && distFront < STOP_DISTANCE_FRONT) {
        currentState = TURN_RIGHT;
        stateStartTime = millis();
      } 
      else if (distLeft > LEFT_LOST_DIST && distLeft != 999) {
        currentState = OFFSET_FORWARD;
        stateStartTime = millis();
      } 
      else {
        int error = TARGET_DIST_LEFT - distLeft; 
        int correction = Kp * error;
        move(LEFT_FORWARD, BASE_SPEED - correction, RIGHT_FORWARD, BASE_SPEED + correction);
      }
      break;

    case TURN_RIGHT:
      move(LEFT_FORWARD, TURN_SPEED, RIGHT_BACKWARD, TURN_SPEED);
      if (millis() - stateStartTime >= TURN_90_TIME) {
        currentState = WALL_FOLLOW;
      }
      break;

    case OFFSET_FORWARD:
      move(LEFT_FORWARD, BASE_SPEED, RIGHT_FORWARD, BASE_SPEED);
      if (millis() - stateStartTime >= OFFSET_TIME) {
        currentState = TURN_LEFT;
        stateStartTime = millis();
      }
      break;

    case TURN_LEFT:
      move(LEFT_BACKWARD, TURN_SPEED, RIGHT_FORWARD, TURN_SPEED);
      if (millis() - stateStartTime >= TURN_90_TIME) {
        currentState = WALL_FOLLOW;
      }
      break;
  }
  delay(30);
}
