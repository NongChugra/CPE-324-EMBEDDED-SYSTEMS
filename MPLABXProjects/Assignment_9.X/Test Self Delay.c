#define F_CPU 1000000UL
#include <avr/io.h>
#include <util/delay.h>

void delay_us_init()
{
    TCCR1A = 0;
    TCCR1B = 0;

    TCCR1B |= (1 << WGM12);   // CTC mode
    TCCR1B |= (1 << CS10);    // prescaler 1 ? 1us per tick

    TCNT1 = 0;
}

void my_delay_us(uint16_t us)
{
    TCNT1 = 0;
    OCR1A = us;

    TIFR1 |= (1 << OCF1A); // clear flag

    while (!(TIFR1 & (1 << OCF1A)));

    TIFR1 |= (1 << OCF1A); // clear flag again
}

void my_delay_ms(uint16_t ms)
{
    while(ms--)
    {
        my_delay_us(1000);
    }
}

int main(void)
{
    DDRB |= (1 << PB1);
    delay_us_init();

    while (1)
    {
        PORTB ^= (1 << PB1);

        // ===== compare here =====

        // Built-in delay
//        _delay_ms(500);

        // Self-made delay (Timer1)
        my_delay_ms(500);
    }
}