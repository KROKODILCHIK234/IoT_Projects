#include <Arduino.h>

const int RX_PIN = 2;     
const int TX_PIN = 8;     
const int LED_PIN = 9;    
const int BTN_PIN = 4;    


const int dotLen = 200;        
const int dashThreshold = 400; 
const int letterGap = 600;     


volatile unsigned long signalStart = 0;
volatile unsigned long lastPulseDuration = 0;
volatile bool pulseReady = false; 
volatile bool signalActive = false; 

String rxBuffer = "";       
unsigned long lastSignalTime = 0; 


String manualBuffer = "";
unsigned long btnPressStart = 0;
bool isBtnPressed = false;
unsigned long lastBtnRelease = 0;

char decode(String s) {
  if (s == ".-") return 'A';   if (s == "-...") return 'B'; if (s == "-.-.") return 'C';
  if (s == "-..") return 'D';  if (s == ".") return 'E';    if (s == "..-.") return 'F';
  if (s == "--.") return 'G';  if (s == "....") return 'H'; if (s == "..") return 'I';
  if (s == ".---") return 'J'; if (s == "-.-") return 'K';  if (s == ".-..") return 'L';
  if (s == "--") return 'M';   if (s == "-.") return 'N';   if (s == "---") return 'O';
  if (s == ".--.") return 'P'; if (s == "--.-") return 'Q'; if (s == ".-.") return 'R';
  if (s == "...") return 'S';  if (s == "-") return 'T';    if (s == "..-") return 'U';
  if (s == "...-") return 'V'; if (s == ".--") return 'W';  if (s == "-..-") return 'X';
  if (s == "-.--") return 'Y'; if (s == "--..") return 'Z';
  if (s == ".----") return '1'; if (s == "..---") return '2'; if (s == "...--") return '3';
  if (s == "....-") return '4'; if (s == ".....") return '5'; if (s == "-....") return '6';
  if (s == "--...") return '7'; if (s == "---..") return '8'; if (s == "----.") return '9';
  if (s == "-----") return '0';
  return '?';
}

const char* letters[] = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--.."
};
const char* numbers[] = {
  "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----."
};

void rxISR() {
  int state = digitalRead(RX_PIN); 

  if (state == HIGH) {

    signalStart = millis();
    signalActive = true;
 
    digitalWrite(LED_PIN, HIGH); 
  } 
  else {

    lastPulseDuration = millis() - signalStart;
    signalActive = false;
    pulseReady = true; 
    digitalWrite(LED_PIN, LOW);
  }
}

void setup() {
  pinMode(RX_PIN, INPUT); 
  pinMode(TX_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  
  Serial.begin(9600);
  Serial.println("--- INTERRUPT MORSE SYSTEM ---");
  Serial.println("Ensure RX is connected to PIN 2!");


  attachInterrupt(digitalPinToInterrupt(RX_PIN), rxISR, CHANGE);
}

void processInterruptData() {

  if (pulseReady) {

    noInterrupts();
    pulseReady = false;
    unsigned long duration = lastPulseDuration;
    interrupts();

    lastSignalTime = millis();


    if (duration > 30) { 
      if (duration < dashThreshold) rxBuffer += ".";
      else rxBuffer += "-";
    }
  }

  if (!signalActive && rxBuffer.length() > 0) {
    if (millis() - lastSignalTime > letterGap) {
      char c = decode(rxBuffer);
      Serial.print("RX (INT): "); Serial.println(c);
      rxBuffer = "";
    }
  }
}

void smartWait(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    processInterruptData(); 
    handleButton(); 
  }
}


void sendPulse(const char* code) {

  detachInterrupt(digitalPinToInterrupt(RX_PIN));

  int i = 0;
  while (code[i] != '\0') {
    digitalWrite(TX_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
    if (code[i] == '.') smartWait(dotLen);
    else smartWait(dotLen * 3);
    digitalWrite(TX_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    smartWait(dotLen);
    i++;
  }
  smartWait(dotLen * 3);
  
  attachInterrupt(digitalPinToInterrupt(RX_PIN), rxISR, CHANGE);
}

void handleButton() {
  int btnVal = digitalRead(BTN_PIN);

  if (btnVal == LOW && !isBtnPressed) {
    isBtnPressed = true;
    btnPressStart = millis();
    digitalWrite(TX_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
  }

  if (btnVal == HIGH && isBtnPressed) {
    isBtnPressed = false;
    unsigned long duration = millis() - btnPressStart;
    digitalWrite(TX_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    lastBtnRelease = millis();

    if (duration > 30) {
      if (duration < dashThreshold) manualBuffer += ".";
      else manualBuffer += "-";
    }
  }

  if (!isBtnPressed && manualBuffer.length() > 0) {
    if (millis() - lastBtnRelease > letterGap) {
      char c = decode(manualBuffer);
      Serial.print("YOU SENT: "); Serial.println(c);
      manualBuffer = "";
    }
  }
}

void loop() {

  processInterruptData();

  handleButton();

  if (Serial.available()) {
    char c = Serial.read();
    c = toupper(c);
    if (c >= 'A' && c <= 'Z') sendPulse(letters[c - 'A']);
    else if (c >= '0' && c <= '9') sendPulse(numbers[c - '0']);
    else if (c == ' ') smartWait(dotLen * 7);
  }
}
