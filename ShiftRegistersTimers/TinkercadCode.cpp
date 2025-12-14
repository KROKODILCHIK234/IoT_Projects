#include <avr/io.h>
#include <avr/interrupt.h>

volatile int g_cnt = 0;
volatile int g_ovr = -1;
volatile bool g_run = false;

volatile uint16_t g_buf = 0;
volatile int g_idx = 15;
volatile int g_ms = 0;

enum St { S_IDLE, S_DATA, S_LATCH };
volatile St g_st = S_IDLE;

const bool PATTERNS[10][8] = {
  {1,1,0,1,1,1,0,1}, {0,1,0,1,0,0,0,0},
  {1,1,0,0,1,1,1,0}, {1,1,0,1,1,0,1,0},
  {0,1,0,1,0,0,1,1}, {1,0,0,1,1,0,1,1},
  {1,0,1,1,1,1,1,1}, {1,1,0,1,0,0,0,0},
  {1,1,0,1,1,1,1,1}, {1,1,1,1,1,0,1,1}
};

void setup() {
  DDRD |= (1 << 3) | (1 << 5) | (1 << 7);
  PORTD &= ~(1 << 3);
  PORTD |= (1 << 5);
  
  Serial.begin(9600);
  Serial.println("RDY");

  cli();
  TCCR1A = 0; TCCR1B = 0; TCNT1 = 0;
  OCR1A = 249; 
  TCCR1B |= (1 << WGM12) | (1 << CS11) | (1 << CS10);
  TIMSK1 |= (1 << OCIE1A);
  sei();
}

void loop() {
  static char b[4]; static byte p = 0;
  if (Serial.available()) {
    char c = Serial.read();
    if (c >= '0' && c <= '9' && p < 2) b[p++] = c;
    if (p == 2 || c == '\n') {
      b[p] = 0; int v = atoi(b);
      if (v >= 0 && v <= 99) {
        cli();
        if (!g_run) { g_cnt = v; g_run = true; g_ms = 999; Serial.println(v); }
        else { g_ovr = v; Serial.println(v); }
        sei();
      }
      p = 0;
    }
  }
}

uint16_t enc(int n) {
  int t = n / 10; int o = n % 10;
  uint16_t res = 0;
  for(int i=7; i>=0; i--) if(PATTERNS[t][i]) res |= (1 << (8 + i));
  for(int i=7; i>=0; i--) if(PATTERNS[o][i]) res |= (1 << i);
  return res;
}

ISR(TIMER1_COMPA_vect) {
  g_ms++;
  if (g_ms >= 1000) {
    g_ms = 0;
    if (g_run) {
      int val;
      if (g_ovr != -1) { val = g_ovr; g_ovr = -1; g_cnt++; } 
      else { val = g_cnt++; }
      
      if (g_cnt > 99) g_cnt = 0;
      g_buf = enc(val);
      g_st = S_DATA; 
      g_idx = 15;
    }
  }

  switch (g_st) {
    case S_DATA:
      if ((g_buf >> g_idx) & 1) PORTD |= (1 << 7); else PORTD &= ~(1 << 7);
      PORTD |= (1 << 3); PORTD &= ~(1 << 3);
      g_idx--;
      if (g_idx < 0) g_st = S_LATCH;
      break;
    case S_LATCH:
      PORTD |= (1 << 5); PORTD &= ~(1 << 5);
      g_st = S_IDLE;
      break;
    case S_IDLE: break;
  }
}
