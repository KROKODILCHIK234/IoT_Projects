#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define BOARD_ID 2

LiquidCrystal_I2C lcd(0x27, 16, 2); 

const int RX_PIN = 7;     
const int TX_PIN = 8;     
const int LED_PIN = 9;    
const int BTN_PIN = 4;    

// Тайминги
const int dotLen = 300; 
const int dashThreshold = dotLen * 1.5; 
const int letterGap = dotLen * 3; 

// Переменные (RX)
String rxBuffer = "";       
unsigned long rxSignalStart = 0;
unsigned long rxSignalEnd = 0;
bool isReceiving = false;
String currentLineText = ""; 

// Переменные ручной отпраки(TX Manual)
// Нужны, чтобы расшифровать свой же стук для лога
String txManualBuffer = "";
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

void setup() {
  pinMode(RX_PIN, INPUT);
  pinMode(TX_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  Serial.begin(9600);

  Serial.print("SYSTEM START | ID: ");
  Serial.print(BOARD_ID);

  if (BOARD_ID == 1) {
    lcd.init(); 
    lcd.backlight();
    lcd.setCursor(0, 0); lcd.print("ID1: Ready");
    lcd.setCursor(0, 1); lcd.print("ID2: Waiting");
    delay(1000);
    lcd.clear();
  } else {
    Wire.begin();
    lcd.setBacklight(HIGH); 
    delay(1500); 
  }
}

void writeToLcd(char c) {
  int targetRow = (BOARD_ID == 1) ? 1 : 0;
  
  currentLineText += c;
  
  // Рандомная задержка для защиты шины I2C,
  // чтобы избежать множественного Master'а
  delay(random(5, 15)); 

  lcd.setCursor(0, targetRow);
  if (currentLineText.length() > 16) {
    lcd.print(currentLineText.substring(currentLineText.length() - 16));
  } else {
    lcd.print(currentLineText);
  }
}

// Проверка входящих
void checkRx() {
  int sensorVal = digitalRead(RX_PIN);
  
  if (sensorVal == HIGH && !isReceiving) {
    rxSignalStart = millis();
    isReceiving = true;
    digitalWrite(LED_PIN, HIGH); 
  }

  if (sensorVal == LOW && isReceiving) {
    isReceiving = false;
    digitalWrite(LED_PIN, LOW);
    unsigned long duration = millis() - rxSignalStart;
    rxSignalEnd = millis();

    if (duration > 30) {
      if (duration < dashThreshold) rxBuffer += ".";
      else rxBuffer += "-";
    }
  }

  if (!isReceiving && rxBuffer.length() > 0) {
    if (millis() - rxSignalEnd > letterGap) {
      char c = decode(rxBuffer);
      
      Serial.print(" [RX] Received: "); 
      Serial.print(c);
      Serial.print(" (Code: ");
      Serial.print(rxBuffer);
      Serial.println(")");
      
      writeToLcd(c);
      rxBuffer = "";
    }
  }
}

// Задержка
void smartWait(unsigned long ms) {
  unsigned long t = millis();
  while (millis() - t < ms) {
    checkRx();
  }
}

// Отправка SERIAL
void sendPulse(const char* code) {
  int i = 0;
  while (code[i] != '\0') {
    digitalWrite(TX_PIN, HIGH);
    if (code[i] == '.') smartWait(dotLen);
    else smartWait(dotLen * 3);
    digitalWrite(TX_PIN, LOW);
    smartWait(dotLen);
    i++;
  }
  smartWait(dotLen * 3);
}

// Ручной ввод
void handleButton() {
  int btnVal = digitalRead(BTN_PIN);

  // Нажатие
  if (btnVal == LOW && !isBtnPressed) {
    isBtnPressed = true;
    btnPressStart = millis();
    digitalWrite(TX_PIN, HIGH);
  }

  // Отпускание
  if (btnVal == HIGH && isBtnPressed) {
    isBtnPressed = false;
    unsigned long duration = millis() - btnPressStart;
    digitalWrite(TX_PIN, LOW);
    lastBtnRelease = millis();

    if (duration > 30) {
      if (duration < dashThreshold) txManualBuffer += ".";
      else txManualBuffer += "-";
    }
  }

  // Декодирование своего стука для лога
  if (!isBtnPressed && txManualBuffer.length() > 0) {
    if (millis() - lastBtnRelease > letterGap) {
      char c = decode(txManualBuffer);
      
      Serial.print(" [TX Manual] Sent: ");
      Serial.println(c);
      
      txManualBuffer = "";
    }
  }
}

void loop() {
  // 1. Слушаем эфир
  checkRx();

  // 2. Обрабатываем кнопку и логируем свои нажатия
  handleButton();

  // 3. Обрабатываем ввод с клавиатуры
  if (Serial.available()) {
    char c = Serial.read();
    c = toupper(c);
    
    if (c >= 'A' && c <= 'Z') {
      Serial.print(" [TX Serial] Sending: "); Serial.println(c);
      sendPulse(letters[c - 'A']);
    } 
    else if (c >= '0' && c <= '9') {
      Serial.print(" [TX Serial] Sending: "); Serial.println(c);
      sendPulse(numbers[c - '0']);
    } 
    else if (c == ' ') {
      Serial.println(" [TX Serial] Sending: SPACE");
      smartWait(dotLen * 7);
    }
  }
}
