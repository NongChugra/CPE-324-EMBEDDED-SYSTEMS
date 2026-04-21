#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

void delay_init()
{
    TCCR2A = 0;
    TCCR2B = 0;
    TCNT2 = 0;

    // CTC Mode (WGM21 = 1)
    TCCR2A |= (1 << WGM21);

    // Prescaler = 8  (Tick = 1us at 8MHz)
    TCCR2B |= (1 << CS21);
}

void my_delay_us(uint16_t us)
{
    while (us > 0)
    {
        uint8_t chunk;

        if (us > 255)
            chunk = 255;
        else
            chunk = us;

        TCNT2 = 0;
        OCR2A = chunk - 1;

        TIFR2 |= (1 << OCF2A); // Clear flag
        while (!(TIFR2 & (1 << OCF2A)))
            ;
        TIFR2 |= (1 << OCF2A); // Clear flag again

        us -= chunk;
    }
}

void my_delay_ms(uint16_t ms)
{
    while (ms--)
    {
        my_delay_us(1000);
    }
}

#define LCD_DATA_PORT PORTC
#define LCD_DATA_DDR DDRC

#define LCD_E_PORT PORTC
#define LCD_E_DDR DDRC
#define LCD_E_PIN PC4

#define LCD_RS_PORT PORTC
#define LCD_RS_DDR DDRC
#define LCD_RS_PIN PC5

void commitData()
{
    LCD_E_PORT |= (1 << LCD_E_PIN);
    my_delay_us(1);
    LCD_E_PORT &= ~(1 << LCD_E_PIN);
    my_delay_us(100);
}

void SendLCDCommand(uint8_t command)
{
    // RS = 0
    LCD_RS_PORT &= ~(1 << LCD_RS_PIN);

    // send high nibble case pin 4-7
    // LCD_DATA_PORT &= 0x0F;
    // LCD_DATA_PORT |= (command & 0xF0);

    // send high nibble case pin 0-3
    LCD_DATA_PORT &= 0xF0;
    LCD_DATA_PORT |= (command >> 4);

    commitData();

    // send low nibble case pin 4-7
    // LCD_DATA_PORT &= 0x0F;
    // LCD_DATA_PORT |= ((command & 0x0F) << 4);

    // send low nibble case pin 0-3
    LCD_DATA_PORT &= 0xF0;
    LCD_DATA_PORT |= (command & 0x0F);

    commitData();

    my_delay_ms(2);
}

void SendLCDData(uint8_t data)
{
    // RS = 1
    LCD_RS_PORT |= (1 << LCD_RS_PIN);

    // send high nibble case pin 4-7
    // LCD_DATA_PORT &= 0x0F;
    // LCD_DATA_PORT |= (data & 0xF0);

    // send low nibble case pin 0-3
    LCD_DATA_PORT &= 0xF0;
    LCD_DATA_PORT |= (data >> 4);

    commitData();

    // send low nibble case pin 4-7
    // LCD_DATA_PORT &= 0x0F;
    // LCD_DATA_PORT |= ((data & 0x0F) << 4);

    // send low nibble case pin 0-3
    LCD_DATA_PORT &= 0xF0;
    LCD_DATA_PORT |= (data & 0x0F);

    commitData();

    my_delay_ms(2);
}

void SendLCDString(const char *str)
{
    while (*str)
    {
        SendLCDData(*str++);
    }
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t address;

    if (row == 0)
        address = 0x80 + col;
    else
        address = 0xC0 + col;

    SendLCDCommand(address);
}

void LCD_InitSequence()
{
    // init sequence
    SendLCDCommand(0x33); // force LCD reset / sync in 8-bit init mode
    SendLCDCommand(0x32); // switch to 4-bit mode

    SendLCDCommand(0x28); // 4-bit, 2-line, 5x8 font
    SendLCDCommand(0x0C); // display on, cursor off
    SendLCDCommand(0x01); // clear screen, cursor to home
    SendLCDCommand(0x06); // cursor moves right, no display shift
    my_delay_ms(2);
}

void LCD_init()
{
    // DATA pins output (PORTx0-PORTx3)
    LCD_DATA_DDR |= 0x0F;
    LCD_DATA_PORT &= ~0x0F;

    // E and RS outputs
    LCD_E_DDR |= (1 << LCD_E_PIN);
    LCD_RS_DDR |= (1 << LCD_RS_PIN);

    LCD_E_PORT &= ~(1 << LCD_E_PIN);
    LCD_RS_PORT &= ~(1 << LCD_RS_PIN);

    my_delay_ms(40);

    LCD_InitSequence();
}

// ROW PORT (OUTPUT)
#define KEYPAD_ROW_PORT PORTD
#define KEYPAD_ROW_DDR DDRD

// COL PORT (INPUT)
#define KEYPAD_COL_PORT PORTD
#define KEYPAD_COL_DDR DDRD
#define KEYPAD_COL_PIN PIND

// -------- CASE B: ROW 0-3 , COL 4-7 --------
#define ROW_MASK 0x0F
#define COL_MASK 0xF0
#define ROW_SHIFT 0
#define COL_SHIFT 4

char keymap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

void keypad_init()
{
    // ROW = OUTPUT
    KEYPAD_ROW_DDR |= ROW_MASK;
    KEYPAD_ROW_PORT |= ROW_MASK; // default HIGH

    // COL = INPUT
    KEYPAD_COL_DDR &= ~COL_MASK;

    // Enable pull-up on COL
    KEYPAD_COL_PORT |= COL_MASK;
}

char keypad_scan()
{
    for (uint8_t row = 0; row < 4; row++)
    {
        // Set all ROW HIGH
        KEYPAD_ROW_PORT |= ROW_MASK;

        // Pull one ROW LOW
        KEYPAD_ROW_PORT &= ~(1 << (row + ROW_SHIFT));

        my_delay_us(5);

        // Read column state
        uint8_t col_state = KEYPAD_COL_PIN & COL_MASK;

        if (col_state != COL_MASK)
        {
            for (uint8_t col = 0; col < 4; col++)
            {
                if (!(col_state & (1 << (col + COL_SHIFT))))
                {
                    my_delay_ms(20); // debounce
                    return keymap[row][col];
                }
            }
        }
    }

    return 0;
}

#define TIMER1_INIT 55536 // 65536 - 55536 = 10000 ticks

volatile uint16_t random_number = 0;
volatile uint16_t Timer = 0;

// toggle LED when overflow
ISR(TIMER1_OVF_vect)
{
    Timer = TCNT1; // reload timer
}

// Timer 1 init
void TIMER1_init()
{
    // INT0 falling edge
    EICRA |= (1 << ISC01) | (0 << ISC00);
    EIMSK |= (1 << INT0);

    // Timer1: Normal mode, prescaler 1024
    TCCR1A = (0 << WGM10) | (0 << WGM11);
    TCCR1B = (1 << CS12) | (1 << CS10);
    TCNT1 = TIMER1_INIT;

    TIMSK1 = (1 << TOIE1); // open Overflow Interrupt
}

int main(void)
{
    delay_init();
    TIMER1_init();
    LCD_init();

    keypad_init();

    // initADC();
    // initPWM();
    // initINT0();

    sei(); // Enable global interrupts

    while (1)
    {
        int Num = 0;
        int Num2 = 0;
        uint8_t Stop = 0;
        uint8_t Stop2 = 0;
        char key;
        char buffer[16];
        uint16_t Range = 0;

        SendLCDString("Start = ");

        while (Stop == 0)
        {

            key = keypad_scan();
            if (key != 0)
            {
                key = keypad_scan();
                Num = Num * 10 + (int)key;

                snprintf(buffer, 16, "%d", Num);
                SendLCDString(buffer);
            }

            if ((key == '*') | (key == '#'))
            {
                Stop = 1;

                SendLCDCommand(0x01);
                SendLCDCommand(0x80);
                SendLCDString("End = ");

                while (Stop2 == 0)
                {

                    key = keypad_scan();
                    if (key != 0)
                    {
                        key = keypad_scan();
                        Num2 = Num2 * 10 + (int)key;

                        snprintf(buffer, 16, "%d", Num2);
                        SendLCDString(buffer);
                    }

                    if ((key == '*') | (key == '#'))
                    {
                        random_number = 65536 - Timer;

                        Range = Num2 - Num;
                        random_number = (random_number / 10000) * Range + Num;

                        snprintf(buffer, 16, "Result = %d", random_number);
                        SendLCDString(buffer);

                        key = keypad_scan();
                        while (key != 0)
                        {
                            key = keypad_scan();

                            if ((key == '*') | (key == '#'))
                            {
                                Stop = 0;
                                Stop2 = 0;
                            }

                            my_delay_ms(1000);
                        }
                    }

                    my_delay_ms(1000);
                }
            }

            my_delay_ms(1000);
        }

        my_delay_ms(1000);
    }
}