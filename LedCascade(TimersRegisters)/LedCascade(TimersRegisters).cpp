#include <avr/io.h>
#include <avr/interrupt.h>

struct LedController {
  const uint8_t pin;
  const int reload_value;
  int countdown;
};

LedController led_array[] = {
  {PB0, 10, 10},
  {PB1, 20, 20},
  {PB2, 30, 30},
  {PB4, 40, 40},
  {PB5, 50, 50}
};

const int number_of_leds = sizeof(led_array) / sizeof(led_array[0]);

void configure_gpio() {
  uint8_t pin_mask = 0;
  for (int i = 0; i < number_of_leds; i++) {
    pin_mask |= (1 << led_array[i].pin);
  }
  DDRB |= pin_mask;
  PORTB &= ~pin_mask;
}

void configure_timer_interrupts() {
  cli();
  
  TCCR1A = 0;
  TCCR1B = 0;
  
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS11) | (1 << CS10);
  
  OCR1A = 9999;
  
  TIMSK1 |= (1 << OCIE1A);
  
  sei();
}

void setup() {
  configure_gpio();
  configure_timer_interrupts();
}

ISR(TIMER1_COMPA_vect) {
  for (int i = 0; i < number_of_leds; i++) {
    led_array[i].countdown--;

    if (led_array[i].countdown == 0) {
      PORTB ^= (1 << led_array[i].pin);
      led_array[i].countdown = led_array[i].reload_value;
    }
  }
}

void loop() {

}
