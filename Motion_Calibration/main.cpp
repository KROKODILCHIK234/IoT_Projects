#include <EEPROM.h>

#define USE_TINKERCAD 1 

#if USE_TINKERCAD
  #define btSerial Serial
#else
  #include <SoftwareSerial.h>
  SoftwareSerial btSerial(2, 3);
#endif

// Пины моторов
#define DIR_LEFT 4
#define SPEED_LEFT 5
#define DIR_RIGHT 7
#define SPEED_RIGHT 6

// Режимы работы
enum Mode {
  DRIVE_MODE = 0,
  CALIB_DIR = 1,
  CALIB_BALANCE = 2,
  CALIB_TURNS = 3
};
Mode currentMode = DRIVE_MODE;

// Константы направлений
const bool dirCombos[4][2] = { {0,0}, {1,0}, {0,1}, {1,1} };

// ПЕРЕМЕННЫЕ КАЛИБРОВКИ
int idxF = 0, idxB = 0, idxL = 0, idxR = 0; // Индексы направлений
int leftOffset = 0;                          // Баланс скорости (0..50)
unsigned long t90 = 500, t180 = 1000, t360 = 2000; // Время поворотов

char activeAct = ' ';
int testIdx = 0;

void setup() {
  Serial.begin(9600);
  #if !USE_TINKERCAD
    btSerial.begin(9600);
  #endif
  
  pinMode(DIR_LEFT, OUTPUT); pinMode(SPEED_LEFT, OUTPUT);
  pinMode(DIR_RIGHT, OUTPUT); pinMode(SPEED_RIGHT, OUTPUT);

  loadSettings(); 
  Serial.println("System Ready. Mode: DRIVE");
  Serial.println("Press '3'(X) to cycle modes.");
}

void move(int combo, int speedL, int speedR, int duration = 0) {
  digitalWrite(DIR_LEFT, dirCombos[combo][0]);
  digitalWrite(DIR_RIGHT, dirCombos[combo][1]);
  analogWrite(SPEED_LEFT, constrain(speedL + leftOffset, 0, 255));
  analogWrite(SPEED_RIGHT, speedR);
  if (duration > 0) {
    delay(duration);
    stop();
  }
}

void stop() { analogWrite(SPEED_LEFT, 0); analogWrite(SPEED_RIGHT, 0); }

// СОХРАНЕНИЕ В ПАМЯТЬ
void saveSettings() {
  EEPROM.put(0, idxF); EEPROM.put(2, idxB); 
  EEPROM.put(4, idxL); EEPROM.put(6, idxR);
  EEPROM.put(8, leftOffset);
  EEPROM.put(10, t90); EEPROM.put(14, t180); EEPROM.put(18, t360);
  Serial.println(">>> SETTINGS SAVED TO EEPROM <<<");
}

void loadSettings() {
  EEPROM.get(0, idxF); EEPROM.get(2, idxB);

  if(idxF > 3 || idxF < 0) idxF = 0; 
}

// ЛОГИКА КАЛИБРОВКИ
void handleCalibration(char cmd) {
  // --- Режим 1: Направления ---
  if (currentMode == CALIB_DIR) {
    if (cmd == 'F' || cmd == 'B' || cmd == 'L' || cmd == 'R') {
      activeAct = cmd; testIdx = 0;
      move(testIdx, 150, 150);
    } else if (cmd == '1') { // Кнопка Y
      testIdx = (testIdx + 1) % 4;
      move(testIdx, 150, 150);
      Serial.print("Test Combo: "); Serial.println(testIdx);
    } else if (cmd == '2') { // Кнопка A
      if (activeAct == 'F') idxF = testIdx;
      if (activeAct == 'B') idxB = testIdx;
      if (activeAct == 'L') idxL = testIdx;
      if (activeAct == 'R') idxR = testIdx;
      stop(); Serial.println("Direction Saved.");
    }
  } 
  // --- Режим 2: Баланс скорости  ---
  else if (currentMode == CALIB_BALANCE) {
    if (cmd == 'F') move(idxF, 150, 150);
    else if (cmd == 'L') { leftOffset -= 5; Serial.print("Offset: "); Serial.println(leftOffset); move(idxF, 150, 150); }
    else if (cmd == 'R') { leftOffset += 5; Serial.print("Offset: "); Serial.println(leftOffset); move(idxF, 150, 150); }
    else if (cmd == '2') { stop(); saveSettings(); }
  }
  // --- Режим 3: Тайминги поворотов ---
  else if (currentMode == CALIB_TURNS) {
    if (cmd == 'L') { move(idxL, 150, 150, t90); Serial.println("Test 90 deg"); }
    else if (cmd == 'R') { move(idxR, 150, 150, t180); Serial.println("Test 180 deg"); }
    else if (cmd == '1') { t90 += 50; Serial.print("T90 duration: "); Serial.println(t90); }
    else if (cmd == 'B') { t90 -= 50; Serial.print("T90 duration: "); Serial.println(t90); }
    else if (cmd == '2') { saveSettings(); }
  }
}

void loop() {
  if (btSerial.available()) {
    char c = btSerial.read();
    
    if (c == '3') { 
      stop();
      currentMode = (Mode)((int)currentMode + 1);
      if (currentMode > CALIB_TURNS) currentMode = DRIVE_MODE;
      Serial.print("\nNEW MODE: "); Serial.println((int)currentMode);
      return;
    }

    if (currentMode == DRIVE_MODE) {
      if (c == 'F') move(idxF, 150, 150);
      else if (c == 'B') move(idxB, 150, 150);
      else if (c == 'L') move(idxL, 150, 150);
      else if (c == 'R') move(idxR, 150, 150);
      else if (c == '4') stop();
    } else {
      handleCalibration(c);
    }
  }
}
