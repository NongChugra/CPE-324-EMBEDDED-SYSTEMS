#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>

void commitData()
{
    PORTB |= (1 << PORTB2);
    _delay_us(1);
    PORTB &= ~(1 << PORTB2);
    _delay_us(100);
}

void SendLCDCommand(uint8_t command)
{
  // Pull RS low
  PORTB &= ~(1 << PORTB1);
  // Send high nibble of the command
  PORTC &= 0xF0;
  PORTC |= command >> 4;
  
  commitData();
  
  // Send low nibble of the command
  PORTC &= 0xF0;
  PORTC |= (command & 0x0F);
  
  commitData();
  
  _delay_ms(2);
}

void SendLCDData(uint8_t command)
{
  // Pull RS high
  PORTB |= (1 << PORTB1);
  // Send high nibble of the data
  PORTC &= 0xF0;
  PORTC |= command >> 4;
  
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
    PORTC &= 0xF0;
    
    DDRB |= (1 << DDB1) | (1 << DDB2);
    PORTB &= ~(1 << PORTB1) & ~(1 << PORTB2);

    _delay_ms(40);

    PORTB &= ~(1 << PB1);  // RS = 0

    // SendLCDCommand(0x33);
    //SendLCDCommand(0x32);
    //SendLCDCommand(0x28);
    // This only work when timing is perfect as simulation, So instead
    
    // Send 0x03 three times (8-bit mode wake-up)
    SendLCDCommand(0x03);
    _delay_ms(5);

    SendLCDCommand(0x03);
    _delay_us(200);

    SendLCDCommand(0x03);
    _delay_us(200);

    // Switch to 4-bit mode
    SendLCDCommand(0x02);
    _delay_us(200);
    
    SendLCDCommand(0x28);
    SendLCDCommand(0x0E);
    SendLCDCommand(0x01);
    SendLCDCommand(0x80);
    _delay_ms(2);
}

void initKey(){
    // Rows output
    DDRD |= 0x0F;

    // Columns input
    DDRD &= 0x0F;

    // Enable pull-ups on columns
    PORTD |= 0xF0;
}

char keymap[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

char scanKeypad()
{
    for (uint8_t row = 0; row < 4; row++)
    {
        PORTD |= 0x0F;           // All rows HIGH
        PORTD &= ~(1 << row);    // One row LOW

        _delay_us(5);

        uint8_t col = PIND & 0xF0;

        if (col != 0xF0)  // Something pressed
        {
            for (uint8_t c = 0; c < 4; c++)
            {
                if (!(col & (1 << (c + 4))))
                {
                    _delay_ms(200);  // debounce
                    return keymap[row][c];
                }
            }
        }
    }
    return 0;
}

int main(void)
{
    initLCD();
    initKey();

    SendLCDCommand(0x80);
    SendLCDData('H');
    SendLCDData('E');
    SendLCDData('L');
    SendLCDData('L');
    SendLCDData('O');
    SendLCDData(' ');

    while (1)
    {
        char key = scanKeypad();

        if (key)
        {
            SendLCDData(key);
        }
    }
}
