# define F_CPU 1000000UL

# include <avr/io.h>
# include <avr/interrupt.h>

volatile uint16_t adcValue;
ISR(ADC_vect) {
    adcValue = ADC;

    // ADC to Percentage using /1024, Percentage to OCR0A using *256, 
    // So ADC to OCR0A is *256/1024 which actually just /4
    uint8_t Duty = ADC >> 2;
    OCR0A = Duty;
    
    // Start Conversion
    ADCSRA |= (1 << ADSC);
}

void InitADC()
{
    ADMUX |= (0 << REFS1) | (1 << REFS0); // AVcc as Reference
    ADMUX |= (1 << MUX0);
    
    ADCSRA |= (1 << ADEN); // ADC Enable
    ADCSRA |= (1 << ADIE); // Interrupt Enable
    sei();
    ADCSRA |= (0 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescalar 8
    
    ADCSRA |= (1 << ADSC); // Start Conversion
}

void InitPWM() {
    TCCR0A = (1 << WGM00) | (1 << WGM01); // Fast PWM Mode
    TCCR0A |= (0 << COM0A0) | (1 << COM0A1); // Clear OC0 on Compare, Set on Top
    
    TCCR0B = (0 << CS00) | (1 << CS01) | (0 << CS02); // Prescalar 8
    
    OCR0A = 0; // Best Practice
}

int main(void) {

    // Output at PD6, PB1 as Light Reference
    DDRD |= (1 << DDD6);
    DDRB |= (1 << DDB1);
    PORTB |= (1 << PORTB1);
    
    InitADC();
    InitPWM();
    
    while(1) {
        
    }
}