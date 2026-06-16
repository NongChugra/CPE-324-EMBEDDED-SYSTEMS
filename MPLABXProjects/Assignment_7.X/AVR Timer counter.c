#define F_CPU 1000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define TIMER1_INIT 63582  // 65536 - 63582 = 2000 ticks ? 2 sec with prescaler 1024

volatile uint8_t State = 1; // 1 = bling 0 = off

ISR(INT0_vect)
{
    State ^= 1;
    if (!State) {
        PORTB &= ~(1 << PORTB1);
    }
    _delay_ms(50);
}

// toggle LED when overflow
ISR(TIMER1_OVF_vect)
{
    TCNT1 = TIMER1_INIT;  // reload timer
    if (State) {
        PORTB ^= (1 << PORTB1);
    }
}

int main(void)
{
    // LED output
    DDRB |= (1 << PORTB1);

    // Button input + pull-up
    DDRD &= ~(1 << PORTD2);
    PORTD |= (1 << PORTD2);

    // INT0 falling edge
    EICRA |= (1 << ISC01) | (0 << ISC00);
    EIMSK |= (1 << INT0);

    // Timer1: Normal mode, prescaler 1024
    TCCR1A = (0 << WGM10) | (0 << WGM11);
    TCCR1B = (1 << CS12) | (1 << CS10);
    TCNT1 = TIMER1_INIT;
    
    TIMSK1 = (1 << TOIE1);  // open Overflow Interrupt
    sei();

    while (1)
    {
    }
}