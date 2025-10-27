#define LED_PIN 13
#define BLINK_RATE 500 // 500 мс

enum LedMode { OFF, ON, BLINK };
LedMode currentMode = OFF;

unsigned long lastBlinkTime = 0;
bool ledCurrentState = LOW;

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    
    switch (command) {
      case '1': // ON
        currentMode = ON;
        digitalWrite(LED_PIN, HIGH);
        Serial.println("STATE_ON");
        break;
        
      case '0': // OFF
        currentMode = OFF;
        digitalWrite(LED_PIN, LOW);
        Serial.println("STATE_OFF");
        break;
        
      case '2': // BLINK
        currentMode = BLINK;
        Serial.println("STATE_BLINKING");
        break;
    }
  }

  if (currentMode == BLINK) {
    handleBlinking();
  }
}

void handleBlinking() {
  if (millis() - lastBlinkTime >= BLINK_RATE) {
    lastBlinkTime = millis();
    ledCurrentState = !ledCurrentState;
    digitalWrite(LED_PIN, ledCurrentState);
  }
}
