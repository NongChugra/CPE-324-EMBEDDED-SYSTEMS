#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

// ================================================================================================================
// BootLoader Check (Blink Code)
// ================================================================================================================

// #define F_CPU 8000000UL
// #include <util/delay.h>
//
// int main(void){
//     DDRB |= (1 << DDB1);
//
//     while(1) {
//         PORTB |= (1 << PORTB1);
//         _delay_ms(1000);
//         PORTB &= ~(1 << PORTB1);
//         _delay_ms(1000);
//     }
// }

// ================================================================================================================
// TIMER0 DELAY MODULE (CTC MODE)
// ================================================================================================================
//
// void delay_init()
// {
//     TCCR0A = 0;
//     TCCR0B = 0;
//     TCNT0 = 0;
//
//     // CTC Mode (WGM01 = 1)
//     TCCR0A |= (1 << WGM01);
//
//     // Prescaler = 8  (Tick = 1us at 8MHz)
//     TCCR0B |= (1 << CS01);
// }
//
// void my_delay_us(uint16_t us)
// {
//     while (us > 0)
//     {
//         uint8_t chunk;
//
//         if (us > 255)
//             chunk = 255;
//         else
//             chunk = us;
//
//         TCNT0 = 0;
//         OCR0A = chunk - 1;
//
//         TIFR0 |= (1 << OCF0A); // clear flag
//         while (!(TIFR0 & (1 << OCF0A)));
//         TIFR0 |= (1 << OCF0A); // clear flag again
//
//         us -= chunk;
//     }
// }

// ================================================================================================================
// TIMER1 DELAY MODULE (CTC MODE)
// ================================================================================================================
//
// void delay_init()
// {
//     TCCR1A = 0;
//     TCCR1B = 0;
//     TCNT1 = 0;
//
//     // Set CTC Mode (WGM12 = 1)
//     TCCR1B |= (1 << WGM12);
//
//     // Prescaler = 8 (Tick = 1 us at 8MHz)
//     TCCR1B |= (1 << CS11);
// }
//
// void my_delay_us(uint16_t us)
// {
//     if (us == 0)
//         return;
//     TCNT1 = 0;
//     OCR1A = us - 1;
//     TIFR1 |= (1 << OCF1A); // Clear flag
//     while (!(TIFR1 & (1 << OCF1A)));
//     TIFR1 |= (1 << OCF1A); // Clear flag again
// }

// ================================================================================================================
// TIMER2 DELAY MODULE (CTC MODE)
// ================================================================================================================

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

// ================= USAGE EXAMPLE =================
// Init --------------------------------------------
// delay_init();

// dealy 500 ms ------------------------------------
// my_delay_ms(500);

// delay 100 us ------------------------------------
// my_delay_us(100);

// ================================================================================================================
// LCD MODULE (4-bit mode)
// ================================================================================================================

// LCD D4 -> PORTx4 -> PORTx0
// LCD D5 -> PORTx5 -> PORTx1
// LCD D6 -> PORTx6 -> PORTx2
// LCD D7 -> PORTx7 -> PORTx3

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
    // DATA pins output (PORTx4-PORTx7)
    // LCD_DATA_DDR |= 0xF0;
    // LCD_DATA_PORT &= ~0xF0;

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

// ================= USAGE EXAMPLE =================
// Init --------------------------------------------
// LCD_init();

// reset, clear and set cursor ---------------------
// SendLCDCommand(0x01); // clear screen, cursor to home
// SendLCDCommand(0x80); // set cursor to line 1 start

// print 2 lines -----------------------------------
// LCD_SetCursor(0, 0);
// SendLCDString("HELLO");
//
// LCD_SetCursor(1, 0);
// SendLCDString("WORLD");

// cursor positions --------------------------------
// LCD_SetCursor(0, 0); // row 0 col 0  (line1 start)
// LCD_SetCursor(1, 0); // row 1 col 0  (line2 start)
// LCD_SetCursor(1, 5); // row 1 col 5  (line2 column 6)

// print at specific column ------------------------
// LCD_SetCursor(0, 3);
// SendLCDString("ABC");
//
// LCD_SetCursor(1, 7);
// SendLCDString("123");

/*
LCD DISPLAY:
   ABC
       123
*/

// print number using snprintf ---------------------
// #include <stdio.h>
//
// char buffer[16];
// int num = 25;
//
// LCD_SetCursor(0, 0);
// SendLCDString("VALUE:");
//
// LCD_SetCursor(0, 6);
// snprintf(buffer, 16, "%d", num);
// SendLCDString(buffer);

/*
VALUE:25
*/

// ================================================================================================================
// KEYPAD MODULE (4x4 MATRIX)
// ================================================================================================================

// Rows = OUTPUT
// Cols = INPUT with pull-up

// Supports 4 cases: (Uncomment ONLY ONE)
//  A) ROW 0-3 , COL 0-3
//  B) ROW 0-3 , COL 4-7
//  C) ROW 4-7 , COL 0-3
//  D) ROW 4-7 , COL 4-7

// ROW PORT (OUTPUT)
#define KEYPAD_ROW_PORT PORTD
#define KEYPAD_ROW_DDR DDRD

// COL PORT (INPUT)
#define KEYPAD_COL_PORT PORTD
#define KEYPAD_COL_DDR DDRD
#define KEYPAD_COL_PIN PIND

// -------- CASE A: ROW 0-3 , COL 0-3 --------
// #define ROW_MASK    0x0F
// #define COL_MASK    0x0F
// #define ROW_SHIFT   0
// #define COL_SHIFT   0

// -------- CASE B: ROW 0-3 , COL 4-7 --------
#define ROW_MASK 0x0F
#define COL_MASK 0xF0
#define ROW_SHIFT 0
#define COL_SHIFT 4

// -------- CASE C: ROW 4-7 , COL 0-3 --------
// #define ROW_MASK    0xF0
// #define COL_MASK    0x0F
// #define ROW_SHIFT   4
// #define COL_SHIFT   0

// -------- CASE D: ROW 4-7 , COL 4-7 --------
// #define ROW_MASK    0xF0
// #define COL_MASK    0xF0
// #define ROW_SHIFT   4
// #define COL_SHIFT   4

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

// ================= USAGE EXAMPLE =================
// Init --------------------------------------------
// keypad_init();

// Scan and print key ------------------------------
// char key = keypad_scan();
// if(key != 0)
// {
//     char buffer[16];
//     snprintf(buffer, 16, "KEY: %c", key);
//     SendLCDString(buffer);
// }

// TIMER RANDOM TIMER1

#define TIMER1_INIT 55536 // 65536 - 55536 = 10000 ticks

volatile uint16_t random_number = 0;

// toggle LED when overflow
ISR(TIMER1_OVF_vect)
{
    TCNT1 = TIMER1_INIT; // reload timer
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
    // All init functions
    delay_init();
    TIMER1_init();
    LCD_init();

    keypad_init();

    // ultrasonic_init();

    // SPI_init();
    // I2C_init();

    // initADC();
    // initPWM();
    // initINT0();

    sei(); // Enable global interrupts

    SendLCDCommand(0x01);
    LCD_SetCursor(0, 0);
    SendLCDString("Random number");

    while (1)
    {
        char key = keypad_scan();
        if (key != 0)
        {
            SendLCDCommand(0x01); // clear screen, cursor to home
            SendLCDCommand(0x80); // set cursor to line 1 start

            random_number = TCNT1;
            random_number ^= (random_number << 7);
            random_number ^= (random_number >> 9);
            random_number ^= (random_number << 8);
        }
        else
        {
            char buffer[16];
            snprintf(buffer, 16, "random number: %d", random_number);
            //            SendLCDString(buffer);
            SendLCDCommand(0x01);
            LCD_SetCursor(0, 0);
            SendLCDString(buffer);
        }

        /* code */
    }
}