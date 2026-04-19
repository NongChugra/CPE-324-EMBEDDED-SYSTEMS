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

    // send high nibble case pin 0-3
    LCD_DATA_PORT &= 0xF0;
    LCD_DATA_PORT |= (command >> 4);

    commitData();

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

    // send low nibble case pin 0-3
    LCD_DATA_PORT &= 0xF0;
    LCD_DATA_PORT |= (data >> 4);

    commitData();

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

#define US_TRIG_PORT PORTB
#define US_TRIG_DDR DDRB
#define US_TRIG_PINNUM PB1

#define US_ECHO_PORT PORTB
#define US_ECHO_DDR DDRB
#define US_ECHO_PINREG PINB
#define US_ECHO_PINNUM PB0 // must be ICP1 pin

volatile uint16_t Distance = 0;
volatile uint8_t DistanceReady = 0;

ISR(TIMER1_CAPT_vect)
{
    static uint16_t startTick = 0; // Must persist after the ISR exits
    uint16_t endTick;
    uint16_t ticks;

    if (TCCR1B & (1 << ICES1)) // Rising edge detected
    {
        startTick = ICR1;
        TCCR1B &= ~(1 << ICES1); // switch to falling edge
    }
    else // Falling edge detected
    {
        endTick = ICR1;

        // Handle overflow case
        if (endTick >= startTick)
            ticks = endTick - startTick;
        else
            ticks = (65535 - startTick) + endTick + 1;

        // ticks = time in us
        // distance_cm = (ticks * 34300) / (2 * 1000000)
        Distance = (ticks * 34300UL) / 2000000UL;

        DistanceReady = 1;
        TCCR1B |= (1 << ICES1); // back to rising edge
    }
}

void ultrasonic_timer1_init()
{
    // Normal mode
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    // Prescaler = 8
    TCCR1B |= (1 << CS11);

    // Rising edge first
    TCCR1B |= (1 << ICES1);

    // Enable input capture interrupt
    TIMSK1 |= (1 << ICIE1);
}

void ultrasonic_init()
{
    // TRIG = OUTPUT
    US_TRIG_DDR |= (1 << US_TRIG_PINNUM);
    US_TRIG_PORT &= ~(1 << US_TRIG_PINNUM); // default LOW

    // ECHO = INPUT
    US_ECHO_DDR &= ~(1 << US_ECHO_PINNUM);

    ultrasonic_timer1_init();
}

void ultrasonic_trigger()
{
    US_TRIG_PORT |= (1 << US_TRIG_PINNUM);
    my_delay_us(10);
    US_TRIG_PORT &= ~(1 << US_TRIG_PINNUM);
}

volatile uint8_t spiByteCount = 0;
volatile uint8_t spiHighByte = 0;
volatile uint8_t spiLowByte = 0;

volatile uint16_t adcValue = 0;
volatile uint8_t adcReady = 0;

void SPI_Init()
{
    // MOSI, SCK, SS output
    DDRB |= (1 << PB3) | (1 << PB5) | (1 << PB2);

    // MISO input
    DDRB &= ~(1 << PB4);

    // SS high (inactive)
    PORTB |= (1 << PB2);

    // SPI Enable, Master, Interrupt Enable
    // SPI2X = 1 SPR1 = 0 SPR0 = 1 -> fosc/8 speed
    SPCR = (1 << SPE) | (1 << MSTR) | (0 << SPR1) | (1 << SPR0) | (1 << SPIE);
    SPSR = (1 << SPI2X);
}

void MCP3201_StartConversion()
{
    adcReady = 0;
    spiByteCount = 0;

    // SS low (start)
    PORTB &= ~(1 << PB2);

    // Start first transfer (dummy byte)
    SPDR = 0x00;
}

ISR(SPI_STC_vect)
{
    uint8_t received = SPDR;

    if (spiByteCount == 0)
    {
        spiHighByte = received;
        spiByteCount++;

        // start second transfer
        SPDR = 0x00;
    }
    else
    {
        spiLowByte = received;

        // SS high (end transaction)
        PORTB |= (1 << PB2);

        // MCP3201 extraction (12-bit)
        // adcValue = [HighByte(4:0) + LowByte(7:0)] >> 1
        adcValue = ((((uint16_t)(spiHighByte & 0x1F)) << 8) | spiLowByte) >> 1;

        adcReady = 1;
    }
}

int main(void)
{
    // All init functions
    delay_init();
    LCD_init();

    keypad_init();
    ultrasonic_init();

    SPI_Init();
    //    I2C_Init();

    sei(); // Enable global interrupts

    while (1)
    {
        SendLCDCommand(0x01);
        SendLCDCommand(0x80);

        if (DistanceReady == 0)
        {
            ultrasonic_trigger();
            my_delay_ms(200);
        }

        if (adcReady == 0)
        {
            MCP3201_StartConversion();
            my_delay_ms(200);
        }

        if (adcReady != 0)
        {
            adcReady = 0;
            // adcValue is (0-4095)
            char buffer[16];
            LCD_SetCursor(0, 0);
            snprintf(buffer, 16, "adcValue: %u", adcValue);
            SendLCDString(buffer);
        }

        if (DistanceReady != 0)
        {
            DistanceReady = 0;
            char buffer[16];
            snprintf(buffer, 16, "Distance: %u cm", Distance);
            LCD_SetCursor(1, 0);
            SendLCDString(buffer);
        }

        char key = keypad_scan();
        if (key != 0)
        {
            SendLCDCommand(0x01);
            SendLCDCommand(0x80);

            char buffer[16];
            snprintf(buffer, 16, "KEY: %c", key);
            SendLCDString(buffer);

            my_delay_ms(3000);
        }

        my_delay_ms(1000);
    }
}