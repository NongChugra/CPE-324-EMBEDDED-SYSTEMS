# define F_CPU 1000000UL

# include <avr/io.h>
# include <avr/interrupt.h>
# include <util/delay.h>
# include <stdio.h>

// PB0 -> ICP1 this is ECHO to sensor
// PB1 is Trig to sensor

volatile uint16_t StartTick, EndTick;
volatile uint8_t MeasurementReady = 0;
uint16_t Distance;

ISR(TIMER1_CAPT_vect) {
    if(TCCR1B & (1 << ICES1)) { // Input Capture Detected
        StartTick = ICR1;
        TCCR1B &= ~(1 << ICES1); // Falling Edge Detection
    } else {
        EndTick = ICR1;
        MeasurementReady = 1;
        TCCR1B |= (1 << ICES1); // Rising Edge Detection
    }
}

void Timer1_Init() {
    TCCR1A |= (0 << WGM12) | (0 << WGM11) | (0 << WGM10); // Normal Mode
    
    TCCR1B |= (0 << CS12) | (1 << CS11) | (0 << CS10); // Prescalar 8
    TCCR1B |= (1 << ICES1); // Rising Edge Detection
    
    TIMSK1 |= (1 << ICIE1); // Input Capture Interrupt Enable 
    sei(); // Global Interrupt
}

void commitData()
{
    PORTD |= (1 << PORTD6);
    _delay_us(1);
    PORTD &= ~(1 << PORTD6);
    _delay_us(100);
}

void SendLCDCommand(uint8_t command)
{
  // Pull RS low
  PORTD &= ~(1 << PORTD7);
  // Send high nibble of the data
  PORTC &= 0xF0;
  PORTC |= (command >> 4);
  
  commitData();
  
  // Send low nibble of the data
  PORTC &= 0xF0;
  PORTC |= (command & 0x0F);
 
  commitData();
  
  _delay_ms(2);
}

void SendLCDData(uint8_t command)
{
  // Pull RS high
  PORTD |= (1 << PORTD7);
  // Send high nibble of the data
  PORTC &= 0xF0;
  PORTC |= (command >> 4);
  
  commitData();
  
  // Send low nibble of the data
  PORTC &= 0xF0;
  PORTC |= (command & 0x0F);
 
  commitData();
  
  _delay_ms(2);
}

void initLCD()
{
    DDRC |= 0x0F;
    PORTC &= ~0x0F;
    
    DDRD |= (1 << DDD6) | (1 << DDD7);
    PORTD &= ~(1 << PORTD6) & ~(1 << PORTD7);

    _delay_ms(40);

    PORTD &= ~(1 << PORTD7);  // RS = 0
    
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

char buffer[16];
void SendLCDString(const char *str) {
  while (*str != '\0') {
    SendLCDData(*str);
    str++;
  }
}

int main(void) {
    
    // Input -> ECHO
    DDRB &= ~(1 << DDB0);
    // Output -> Trig
    DDRB |= (1 << DDB1);
    
    Timer1_Init();
    initLCD();
    
    SendLCDCommand(0x80);
    SendLCDString("Hello");
    
    _delay_ms(200);
    
    while (1) {
    
        // Start Detect Distance, Trigger HC-SR04 using 10 µs pulse
        PORTB |= (1 << PORTB1);
        _delay_us(10);
        PORTB &= ~(1 << PORTB1);
    
        _delay_ms(100);
        
        if(MeasurementReady) {
            MeasurementReady = 0;
            
            Distance = ((EndTick - StartTick) * 34000UL) / 250000UL;
            // * 340 m/s * (1 / (1000000 / N)) s  * 100 cm/m and /2 since It time take on both go and back

            SendLCDCommand(0x01);
            SendLCDCommand(0x80);
            
            sprintf(buffer, "%u", Distance);
            SendLCDString("Distance: "); 
            SendLCDString(buffer);
            SendLCDString("cm");
        }
    }
}