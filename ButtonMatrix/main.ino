#include <avr/io.h>
#include <avr/interrupt.h>

const uint8_t KEY_ROWS = 3;
const uint8_t KEY_COLS = 3;

volatile uint16_t activeKeyMap = 0;
volatile uint8_t currentRowIdx = 0;

uint32_t keyPressTimestamps[9];
uint16_t storedKeyMap = 0;

void setup() {
  Serial.begin(9600);

  DDRD |= 0x1C;
  DDRD &= ~0xE0;

  PORTD |= 0x1C; 
  PORTD |= 0xE0;

  cli();
  TCCR1A = 0;
  TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);
  OCR1A = 1249;
  TIMSK1 = (1 << OCIE1A);
  sei();

  Serial.println("System Ready. Tracking Duration & Start Time.");
}

void loop() {
  uint16_t currentSnap;
  
  cli();
  currentSnap = activeKeyMap;
  sei();

  if (currentSnap != storedKeyMap) {
    uint32_t now = millis();

    for (uint8_t i = 0; i < (KEY_ROWS * KEY_COLS); i++) {
      bool active = (currentSnap >> i) & 0x01;
      bool previous = (storedKeyMap >> i) & 0x01;

      if (active && !previous) {
        keyPressTimestamps[i] = now;
      } else if (!active && previous) {
        uint32_t duration = now - keyPressTimestamps[i];
        Serial.print(">> Button ");
        Serial.print(i + 1);
        Serial.print(" released. Duration: ");
        Serial.print(duration);
        Serial.print(" ms. Started at: ");
        Serial.print(keyPressTimestamps[i]);
        Serial.println(" ms.");
      }
    }

    if (currentSnap == 0) {
      Serial.println("Status: All buttons released.");
    } else {
      Serial.print("Status: Buttons pressed [ ");
      bool hasPrev = false;
      for (uint8_t k = 0; k < 9; k++) {
        if ((currentSnap >> k) & 1) {
          if (hasPrev) Serial.print(", ");
          Serial.print(k + 1);
          hasPrev = true;
        }
      }
      Serial.println(" ]");
    }

    storedKeyMap = currentSnap;
  }

  delay(50);
}

ISR(TIMER1_COMPA_vect) {
  PORTD |= (1 << (currentRowIdx + 2));

  currentRowIdx++;
  if (currentRowIdx >= KEY_ROWS) {
    currentRowIdx = 0;
  }

  PORTD &= ~(1 << (currentRowIdx + 2));

  uint8_t portVal = PIND;

  for (uint8_t c = 0; c < KEY_COLS; c++) {
    uint8_t keyId = (currentRowIdx * KEY_COLS) + c;
    
    if (!(portVal & (1 << (c + 5)))) {
      activeKeyMap |= (1 << keyId);
    } else {
      activeKeyMap &= ~(1 << keyId);
    }
  }
}
