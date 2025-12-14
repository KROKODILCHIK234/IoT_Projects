#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>

const uint8_t DIGITS_CC[] = {
    0b00111111,
    0b00000110,
    0b01011011,
    0b01001111,
    0b01100110,
    0b11011101,
    0b01111101,
    0b00000111,
    0b01111111,
    0b01101111
};

volatile uint8_t g_seconds = 0;
volatile uint8_t g_shadow_seconds = 0;
volatile uint8_t g_override_val = 255;
volatile bool g_is_running = false;

volatile uint16_t g_shift_buffer = 0;
volatile int8_t g_bit_index = -1;
volatile uint8_t g_state_flags = 0;

volatile uint16_t g_ms_counter = 0;

void setup_timer1();
uint16_t encode_digits(uint8_t number);

void setup() {
    DDRB |= (1 << DDB0) | (1 << DDB1) | (1 << DDB2);
    PORTB &= ~((1 << PORTB1) | (1 << PORTB2));
    
    Serial.begin(9600);
    Serial.println("System Ready. Enter start value (00-99):");

    setup_timer1();
}

void loop() {
    static char buf[4];
    static uint8_t buf_idx = 0;

    if (Serial.available() > 0) {
        char c = Serial.read();
        
        if (c >= '0' && c <= '9') {
            if (buf_idx < 2) {
                buf[buf_idx++] = c;
            }
        } 
        else if (c == '\n' || c == '\r') {
            if (buf_idx > 0) {
                buf[buf_idx] = '\0';
                int val = atoi(buf);
                if (val >= 0 && val <= 99) {
                    cli();
                    if (!g_is_running) {
                        g_seconds = val;
                        g_is_running = true;
                        g_ms_counter = 999;
                        Serial.print("Started at: ");
                        Serial.println(val);
                    } else {
                        g_override_val = val;
                        Serial.print("Override pending: ");
                        Serial.println(val);
                    }
                    sei();
                }
                buf_idx = 0;
            }
        }
    }
}

void setup_timer1() {
    cli();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1  = 0;

    OCR1A = 249;
    
    TCCR1B |= (1 << WGM12);
    TCCR1B |= (1 << CS11) | (1 << CS10);
    TIMSK1 |= (1 << OCIE1A);
    sei();
}

uint16_t encode_digits(uint8_t number) {
    if (number > 99) number = 99;
    uint8_t tens = number / 10;
    uint8_t ones = number % 10;
    
    uint16_t bits = 0;
    bits |= (uint16_t)DIGITS_CC[ones] << 8;
    bits |= DIGITS_CC[tens];
    
    return bits;
}

ISR(TIMER1_COMPA_vect) {
    g_ms_counter++;

    if (g_ms_counter >= 1000) {
        g_ms_counter = 0;
        
        if (g_is_running) {
            if (g_override_val != 255) {
                g_shadow_seconds = g_override_val;
                g_override_val = 255;
                g_seconds++; 
            } else {
                g_shadow_seconds = g_seconds;
                g_seconds++;
            }
            
            if (g_seconds > 99) g_seconds = 0;

            g_shift_buffer = encode_digits(g_shadow_seconds);
            g_bit_index = 15;
        }
    }

    if (g_bit_index >= 0) {
        if ((g_shift_buffer >> g_bit_index) & 0x01) {
            PORTB |= (1 << 0);
        } else {
            PORTB &= ~(1 << 0);
        }

        PORTB |= (1 << 1);
        PORTB &= ~(1 << 1);
        
        g_bit_index--;

        if (g_bit_index < 0) {
            PORTB |= (1 << 2);
            PORTB &= ~(1 << 2);
        }
    }
}
