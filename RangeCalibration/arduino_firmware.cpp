const int trig_pin = 9;
const int echo_pin = 10;
const int ir_pin = A0;   
const int led_pin = 13;

bool isCalibrating = false; 

void setup() {
  Serial.begin(9600);           
  pinMode(trig_pin, OUTPUT);    
  pinMode(echo_pin, INPUT);     
  pinMode(led_pin, OUTPUT);     
  digitalWrite(led_pin, LOW);   
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n'); 
    command.trim(); 

    if (command == "START") {
      isCalibrating = true;
      digitalWrite(led_pin, LOW); 
      Serial.println("Calibration started...");
    } 
    else if (command == "STOP") {
      isCalibrating = false;
      digitalWrite(led_pin, HIGH); 
      Serial.println("Calibration stopped.");
    }
  }

  if (isCalibrating) {
    digitalWrite(trig_pin, LOW);
    delayMicroseconds(2);
    digitalWrite(trig_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig_pin, LOW);
    
    unsigned long duration = pulseIn(echo_pin, HIGH);
    float distance_cm = duration * 0.0343 / 2.0; 

    int analog_val = analogRead(ir_pin);

    Serial.print(distance_cm);
    Serial.print(",");
    Serial.println(analog_val);

    delay(100); 
  }
}
