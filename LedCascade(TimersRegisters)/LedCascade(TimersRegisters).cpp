#include <avr/io.h>
#include <avr/interrupt.h>

#define LED_COUNT 5

const uint8_t led_portb_pins[LED_COUNT] = {PB0, PB1, PB2, PB4, PB5};
const int blink_tick_rates[LED_COUNT] = {10, 20, 30, 40, 50};

volatile unsigned long master_tick_count = 0;

void setup() {
  uint8_t portb_init_mask = 0;
  for (int i = 0; i < LED_COUNT; i++) {
    portb_init_mask |= (1 << led_portb_pins[i]);
  }

  DDRB |= portb_init_mask;
  PORTB &= ~portb_init_mask;

  cli();

  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS11) | (1 << CS10);
  
  OCR1A = 9999;
  
  TIMSK1 |= (1 << OCIE1A);

  sei();
}

ISR(TIMER1_COMPA_vect) {
  master_tick_count++;

  for (int i = 0; i < LED_COUNT; i++) {
    if ((master_tick_count % blink_tick_rates[i]) == 0) {
      PORTB ^= (1 << led_portb_pins[i]);
    }
  }
}

void loop() {
  
}
