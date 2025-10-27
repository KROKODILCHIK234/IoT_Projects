#define LDR_PIN A0
#define STREAM_INTERVAL 2000 // 2 секунды

unsigned long lastStreamTime = 0;
bool isContinuousMode = false;

void setup() {
  Serial.begin(9600);
  pinMode(LDR_PIN, INPUT);
}

void loop() {
  handleSerialCommands();
  
  if (isContinuousMode) {
    streamLuminosityData();
  }
}

void handleSerialCommands() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    
    switch (command) {
      case 'r': // Read once
        isContinuousMode = false;
        reportLuminosity();
        break;
        
      case 'c': // Continuous stream
        isContinuousMode = true;
        Serial.println("STREAM_MODE_ON");
        break;
    }
  }
}

void streamLuminosityData() {
  if (millis() - lastStreamTime >= STREAM_INTERVAL) {
    lastStreamTime = millis();
    reportLuminosity();
  }
}


void reportLuminosity() {
  int luminosityValue = analogRead(LDR_PIN);
  Serial.print("LDR;");
  Serial.println(luminosityValue);
}
