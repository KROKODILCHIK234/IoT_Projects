#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#define SERIAL_OUTPUT_PIN PD3
#define SERIAL_INPUT_PIN  PD2

#define OUTGOING_QUEUE_CAPACITY 64
#define INCOMING_QUEUE_CAPACITY 64

typedef struct {
    volatile uint8_t write_pos;
    volatile uint8_t read_pos;
    char data_array[INCOMING_QUEUE_CAPACITY];
} CircularQueue;

CircularQueue inbound_queue = {0, 0};
CircularQueue outbound_queue = {0, 0};

volatile uint16_t bit_duration_ticks;

enum TransmissionPhase { 
    PHASE_INACTIVE,
    PHASE_START,
    PHASE_PAYLOAD,
    PHASE_STOP
};
volatile TransmissionPhase current_tx_phase = PHASE_INACTIVE;
volatile uint8_t outgoing_char;
volatile uint8_t tx_bit_counter;

enum ReceptionPhase { 
    PHASE_WAITING,
    PHASE_SAMPLING,
    PHASE_FINALIZE
};
volatile ReceptionPhase current_rx_phase = PHASE_WAITING;
volatile uint8_t incoming_char;
volatile uint8_t rx_bit_counter;

void soft_uart_initialize(uint16_t baud_rate) {
    uint32_t prescaler = 8;
    bit_duration_ticks = (F_CPU / prescaler) / baud_rate;

    DDRD |= (1 << SERIAL_OUTPUT_PIN);
    PORTD |= (1 << SERIAL_OUTPUT_PIN);

    DDRD &= ~(1 << SERIAL_INPUT_PIN);
    PORTD |= (1 << SERIAL_INPUT_PIN);

    TCCR1A = 0;
    TCCR1B = (1 << CS11);

    EICRA |= (1 << ISC01);
    EICRA &= ~(1 << ISC00);
    EIMSK |= (1 << INT0); 

    sei();
}

void soft_uart_transmit_byte(char character) {
    uint8_t next_write_pos = (outbound_queue.write_pos + 1) % OUTGOING_QUEUE_CAPACITY;
    while (next_write_pos == outbound_queue.read_pos) {
    }
    outbound_queue.data_array[outbound_queue.write_pos] = character;
    outbound_queue.write_pos = next_write_pos;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (current_tx_phase == PHASE_INACTIVE) {
            current_tx_phase = PHASE_START;
            OCR1B = TCNT1 + 10;
            TIMSK1 |= (1 << OCIE1B);
        }
    }
}

void soft_uart_print(const char *text) {
    while (*text) {
        soft_uart_transmit_byte(*text++);
    }
}

uint8_t soft_uart_data_waiting() {
    uint8_t count;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        count = (inbound_queue.write_pos - inbound_queue.read_pos + INCOMING_QUEUE_CAPACITY) % INCOMING_QUEUE_CAPACITY;
    }
    return count;
}

char soft_uart_receive_byte() {
    if (inbound_queue.write_pos == inbound_queue.read_pos) {
        return -1;
    }
    char data = inbound_queue.data_array[inbound_queue.read_pos];
    inbound_queue.read_pos = (inbound_queue.read_pos + 1) % INCOMING_QUEUE_CAPACITY;
    return data;
}

bool soft_uart_read_line(char *destination_buffer, uint8_t max_len) {
    if (!soft_uart_data_waiting()) return false;
    uint8_t i = 0;
    while (soft_uart_data_waiting() && i < (max_len - 1)) {
        destination_buffer[i++] = soft_uart_receive_byte();
    }
    destination_buffer[i] = '\0';
    return true;
}

ISR(INT0_vect) {
    EIMSK &= ~(1 << INT0);

    current_rx_phase = PHASE_SAMPLING;
    rx_bit_counter = 0;
    incoming_char = 0;

    OCR1A = TCNT1 + (bit_duration_ticks * 3 / 2);
    TIMSK1 |= (1 << OCIE1A);
}

ISR(TIMER1_COMPA_vect) {
    OCR1A += bit_duration_ticks;

    switch (current_rx_phase) {
        case PHASE_SAMPLING:
            if (PIND & (1 << SERIAL_INPUT_PIN)) {
                incoming_char |= (1 << rx_bit_counter);
            }
            rx_bit_counter++;

            if (rx_bit_counter >= 8) {
                current_rx_phase = PHASE_FINALIZE;
            }
            break;

        case PHASE_FINALIZE:
            if (PIND & (1 << SERIAL_INPUT_PIN)) {
                uint8_t next_head = (inbound_queue.write_pos + 1) % INCOMING_QUEUE_CAPACITY;
                if (next_head != inbound_queue.read_pos) {
                    inbound_queue.data_array[inbound_queue.write_pos] = incoming_char;
                    inbound_queue.write_pos = next_head;
                } 
            }
            current_rx_phase = PHASE_WAITING;
            TIMSK1 &= ~(1 << OCIE1A); 
            EIFR |= (1 << INTF0);     
            EIMSK |= (1 << INT0);     
            break;

        default:
             current_rx_phase = PHASE_WAITING; 
             TIMSK1 &= ~(1 << OCIE1A);
             EIMSK |= (1 << INT0);
             break;
    }
}

ISR(TIMER1_COMPB_vect) {
    OCR1B += bit_duration_ticks;
    
    switch (current_tx_phase) {
        case PHASE_START:
            PORTD &= ~(1 << SERIAL_OUTPUT_PIN);
            
            outgoing_char = outbound_queue.data_array[outbound_queue.read_pos];
            outbound_queue.read_pos = (outbound_queue.read_pos + 1) % OUTGOING_QUEUE_CAPACITY;

            tx_bit_counter = 0;
            current_tx_phase = PHASE_PAYLOAD;
            break;

        case PHASE_PAYLOAD:
            if (outgoing_char & (1 << tx_bit_counter)) {
                PORTD |= (1 << SERIAL_OUTPUT_PIN);
            } else {
                PORTD &= ~(1 << SERIAL_OUTPUT_PIN);
            }
            tx_bit_counter++;
            
            if (tx_bit_counter >= 8) {
                current_tx_phase = PHASE_STOP;
            }
            break;

        case PHASE_STOP:
            PORTD |= (1 << SERIAL_OUTPUT_PIN);
            current_tx_phase = PHASE_INACTIVE;
            break;

        case PHASE_INACTIVE:
            if (outbound_queue.write_pos != outbound_queue.read_pos) {
                current_tx_phase = PHASE_START;
            } else {
                TIMSK1 &= ~(1 << OCIE1B);
            }
            break;
    }
}

void setup() {
    soft_uart_initialize(9600);
    soft_uart_print("Software UART Initialized.\nSend me something!\n");
}

void loop() {
    if (soft_uart_data_waiting()) {
        char received_char = soft_uart_receive_byte();
        
        soft_uart_print("Echo: ");
        soft_uart_transmit_byte(received_char);
        soft_uart_print("\n");
    }
}
