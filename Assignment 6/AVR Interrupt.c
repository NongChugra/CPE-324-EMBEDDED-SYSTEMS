#define F_CPU 1000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

volatile uint8_t State = 2;

ISR(INT0_vect)
{
    State ^= 2; // Toggle state
    PORTB = (State & PORTB);
    _delay_ms(50);
}

int main(void)
{
    DDRB |= (1 << PORTB1);

    DDRD &= ~(1 << PORTD2);
    PORTD |= (1 << PORTD2);

    // INT0 falling edge
    EICRA |= (1 << ISC01) | (0 << ISC00);

    // enable INT0
    EIMSK |= (1 << INT0);

    // Global interrupt
    sei();

    while (1)
    {
        if(State) 
        {
            PORTB ^= (1 << PORTB1);
            _delay_ms(1500);
        } else {
            PORTB &= ~(1 << PORTB1);
        }
    }
}

