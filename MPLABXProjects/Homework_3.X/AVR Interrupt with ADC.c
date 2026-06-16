# define F_CPU 8000000UL

# include <avr/io.h>
# include <util/delay.h>
# include <avr/interrupt.h>
# include <stdio.h>

void commitData()
{
    PORTD |= (1 << PORTD4);
    _delay_us(1);
    PORTD &= ~(1 << PORTD4);
    _delay_us(100);
}

void SendLCDCommand(uint8_t command)
{
  // Pull RS low
  PORTD &= ~(1 << PORTD2);
  // Send high nibble of the data
  PORTB &= 0xE1;
  PORTB |= (command >> 4) << 1;
  
  commitData();
  
  // Send low nibble of the data
  PORTB &= 0xE1;
  PORTB |= ((command & 0x0F) << 1);
 
  commitData();
  
  _delay_ms(2);
}

void SendLCDData(uint8_t command)
{
  // Pull RS high
  PORTD |= (1 << PORTD2);
  // Send high nibble of the data
  PORTB &= 0xE1;
  PORTB |= (command >> 4) << 1;
  
  commitData();
  
  // Send low nibble of the data
  PORTB &= 0xE1;
  PORTB |= ((command & 0x0F) << 1);
 
  commitData();
  
  _delay_ms(2);
}

void initLCD()
{
    DDRB |= 0x1E;
    PORTB &= ~0x1E;
    
    DDRD |= (1 << DDD2) | (1 << DDD4);
    PORTD &= ~(1 << PORTD2) & ~(1 << PORTD4);

    _delay_ms(40);

    PORTD &= ~(1 << PORTD2);  // RS = 0
    
    SendLCDCommand(0x03);
    _delay_ms(5);
    SendLCDCommand(0x03);
    _delay_us(200);
    SendLCDCommand(0x03);
    _delay_us(200);
    SendLCDCommand(0x02);
    _delay_us(200);
    
    SendLCDCommand(0x28);
    SendLCDCommand(0x0E);
    SendLCDCommand(0x01);
    SendLCDCommand(0x80);
    _delay_ms(2);
}

void initADC()
{
    ADMUX |= (1 << REFS0); 
    // enable ADC, ADC Interrupt Enable, and set prescaler to 128
    ADCSRA |= (1 << ADEN) | (1 << ADIE) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

char buffer[16];
void sendLCDString(const char *str) {
  while (*str != '\0') {
    SendLCDData(*str);
    str++;
  }
}

volatile uint16_t adcValue = 0;
volatile uint8_t adcReady = 0;
ISR(ADC_vect) {
  adcValue = ADC; 
  adcReady = 1;   
}

int main(void)
{
    initLCD();
    initADC();

    SendLCDCommand(0x80);
    sendLCDString("Hello");
    
    sei(); // enable global interrupt
  
    // start first conversion
    ADCSRA |= (1 << ADSC); 

    while (1)
    {
        if (adcReady)
        {
            adcReady = 0; // reset flag

            SendLCDCommand(0x01);
            SendLCDCommand(0x80);
            
            // convert to percentage
            uint16_t lightPercent = ((uint32_t)adcValue * 100) / 1023;

            sprintf(buffer, "%u", lightPercent);
            sendLCDString("Light: ");
            sendLCDString(buffer);
            sendLCDString("%");

            _delay_ms(100);

            // start next conversion
            ADCSRA |= (1 << ADSC);
        }
    }
}