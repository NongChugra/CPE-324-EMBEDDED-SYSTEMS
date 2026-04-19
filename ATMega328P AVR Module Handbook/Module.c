#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

// ==========================================================
// BootLoader Check (Blink Code)
// ==========================================================

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

// ==========================================================
// TIMER0 DELAY MODULE (CTC MODE)
// ==========================================================
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

// ==========================================================
// TIMER1 DELAY MODULE (CTC MODE)
// ==========================================================
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

// ==========================================================
// TIMER2 DELAY MODULE (CTC MODE)
// ==========================================================

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

// ==========================================================
// LCD MODULE (4-bit mode)
// ==========================================================

// LCD D4 -> PORTx4 -> PORTx0
// LCD D5 -> PORTx5 -> PORTx1
// LCD D6 -> PORTx6 -> PORTx2
// LCD D7 -> PORTx7 -> PORTx3

#define LCD_DATA_PORT PORTC
#define LCD_DATA_DDR DDRC

#define LCD_E_PORT PORTB
#define LCD_E_DDR DDRB
#define LCD_E_PIN PB6

#define LCD_RS_PORT PORTB
#define LCD_RS_DDR DDRB
#define LCD_RS_PIN PB7

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
    LCD_DATA_PORT &= 0x0F;
    LCD_DATA_PORT |= (command & 0xF0);

    // send high nibble case pin 0-3
    // LCD_DATA_PORT &= 0xF0;
    // LCD_DATA_PORT |= (command >> 4);

    commitData();

    // send low nibble case pin 4-7
    LCD_DATA_PORT &= 0x0F;
    LCD_DATA_PORT |= ((command & 0x0F) << 4);

    // send low nibble case pin 0-3
    // LCD_DATA_PORT &= 0xF0;
    // LCD_DATA_PORT |= (command & 0x0F);

    commitData();

    my_delay_ms(2);
}

void SendLCDData(uint8_t data)
{
    // RS = 1
    LCD_RS_PORT |= (1 << LCD_RS_PIN);

    // send high nibble case pin 4-7
    LCD_DATA_PORT &= 0x0F;
    LCD_DATA_PORT |= (data & 0xF0);

    // send low nibble case pin 0-3
    // LCD_DATA_PORT &= 0xF0;
    // LCD_DATA_PORT |= (data >> 4);

    commitData();

    // send low nibble case pin 4-7
    LCD_DATA_PORT &= 0x0F;
    LCD_DATA_PORT |= ((data & 0x0F) << 4);

    // send low nibble case pin 0-3
    // LCD_DATA_PORT &= 0xF0;
    // LCD_DATA_PORT |= (data & 0x0F);

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
    LCD_DATA_DDR |= 0xF0;
    LCD_DATA_PORT &= ~0xF0;

    // DATA pins output (PORTx0-PORTx3)
    // LCD_DATA_DDR |= 0x0F;
    // LCD_DATA_PORT &= ~0x0F;

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
// snprintf(buffer, sizeof(buffer), "%d", num);
// SendLCDString(buffer);

/*
VALUE:25
*/

// ==========================================================
// KEYPAD MODULE (4x4 MATRIX)
// ==========================================================

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
#define KEYPAD_COL_PORT PORTC
#define KEYPAD_COL_DDR DDRC
#define KEYPAD_COL_PIN PINC

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
                    my_delay_ms(200); // debounce
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

// ==========================================================
// ULTRASONIC SENSOR MODULE (HC-SR04)
// ==========================================================

// ULTRASONIC pins:
// TRIG -> PB1 (OUTPUT)
// ECHO -> PB0 (INPUT, ICP1)

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
        StartTick = ICR1;
        TCCR1B &= ~(1 << ICES1); // switch to falling edge
    }
    else // Falling edge detected
    {
        EndTick = ICR1;

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

// ================= USAGE EXAMPLE =================
// Init --------------------------------------------
// ultrasonic_init();

// Trigger, read and print distance ----------------
// ultrasonic_trigger();
// if(DistanceReady != 0)
// {
//     DistanceReady = 0;
//     char buffer[16];
//     snprintf(buffer, 16, "Distance: %u cm", Distance);
//     SendLCDString(buffer);
// }

// ==========================================================
// SPI MODULE (ATMega328P + MCP3201 ADC)
// ==========================================================

// adcValue ready (0-4095)

// SPI pins:
//   MOSI = PB3
//   MISO = PB4
//   SCK  = PB5
//   SS   = PB2   (as CS for MCP3201)

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
    MCP3201_CS_PORT &= ~(1 << MCP3201_CS_PIN);

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
        MCP3201_CS_PORT |= (1 << MCP3201_CS_PIN);

        // MCP3201 extraction (12-bit)
        // adcValue = [HighByte(4:0) + LowByte(7:0)] >> 1
        adcValue = ((((uint16_t)(spiHighByte & 0x1F)) << 8) | spiLowByte) >> 1;

        adcReady = 1;
    }
}

// ================= USAGE EXAMPLE =================
// Init --------------------------------------------
// SPI_Init();

// Start conversion, read and print value ----------
// MCP3201_StartConversion();
// if (adcReady != 0)
// {
//     adcReady = 0;
//     // adcValue is (0-4095)
//     char buffer[16];
//     snprintf(buffer, 16, "adcValue: %u", adcValue);
//     SendLCDString(buffer);
// }

// ==========================================================
// I2C (TWI) MODULE (DS1307 RTC + ATmega328P)
// ==========================================================

// I2C pins:
//   SCL = PC5
//   SDA = PC4

// DS1307 I2C Address (7-bit)
#define DS1307_ADDRESS 0x68

void I2C_Init(void)
{
    // SCL = F_CPU / (16 + 2 * TWBR * Prescaler)
    // F_CPU = 8MHz, TWBR = 32, Prescaler = 1
    // SCL = 8MHz / (16 + 2 * 32) = 8MHz / 80 = 100kHz
    TWBR = 32;   // Bit rate register
    TWSR = 0x00; // Prescaler = 1

    TWCR = (1 << TWEN); // Enable TWI
}

void I2C_Start(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTA);

    while (!(TWCR & (1 << TWINT)))
        ;
}

void I2C_Stop(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);

    // Wait a little to complete stop condition
    my_delay_us(10);
}

void I2C_Write(uint8_t data)
{
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);

    while (!(TWCR & (1 << TWINT)))
        ;
}

uint8_t I2C_Read_ACK(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);

    while (!(TWCR & (1 << TWINT)))
        ;

    return TWDR;
}

uint8_t I2C_Read_NACK(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN);

    while (!(TWCR & (1 << TWINT)))
        ;

    return TWDR;
}

// DS1307 stores time in BCD format
uint8_t BCD_to_DEC(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

uint8_t DEC_to_BCD(uint8_t dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}

// s  = seconds (0-59)
// m  = minutes (0-59)
// h  = hours   (0-23)
// d  = day of week (1-7)
// dt = date (1-31)
// mt = month (1-12)
// y  = year (0-99)

void DS1307_SetTime(uint8_t s, uint8_t m, uint8_t h,
                    uint8_t d, uint8_t dt,
                    uint8_t mt, uint8_t y)
{
    I2C_Start();
    I2C_Write(DS1307_ADDRESS << 1); // Slave Address + 0 for write 8 bit
    I2C_Write(0x00);                // Start at register 0

    // Bit7 of seconds register = CH (Clock Halt)
    // Must be 0 to run oscillator
    I2C_Write(DEC_to_BCD(s) & 0x7F);

    I2C_Write(DEC_to_BCD(m));
    I2C_Write(DEC_to_BCD(h)); // 24-hour mode
    I2C_Write(DEC_to_BCD(d));
    I2C_Write(DEC_to_BCD(dt));
    I2C_Write(DEC_to_BCD(mt));
    I2C_Write(DEC_to_BCD(y));

    I2C_Stop();
}

void DS1307_ReadTime(uint8_t *sec, uint8_t *min, uint8_t *hour,
                     uint8_t *date, uint8_t *month, uint8_t *year)
{
    // Set DS1307 register pointer to 0x00
    I2C_Start();
    I2C_Write(DS1307_ADDRESS << 1); // Slave Address + 0 for write 8 bit
    I2C_Write(0x00);                // Start at seconds register

    // Repeated start to read
    I2C_Start();
    I2C_Write((DS1307_ADDRESS << 1) | 1); // SLA+R

    *sec = BCD_to_DEC(I2C_Read_ACK() & 0x7F);
    *min = BCD_to_DEC(I2C_Read_ACK());
    *hour = BCD_to_DEC(I2C_Read_ACK() & 0x3F);

    I2C_Read_ACK(); // Skip day of week
    // *day  = BCD_to_DEC(I2C_Read_ACK()); // <-- Day of week

    *date = BCD_to_DEC(I2C_Read_ACK());
    *month = BCD_to_DEC(I2C_Read_ACK());
    *year = BCD_to_DEC(I2C_Read_NACK());

    I2C_Stop();
}

// ================= USAGE EXAMPLE =================
// Init --------------------------------------------
// I2C_Init();

// set time and read continuously ------------------
// char buffer[17];
// uint8_t sec, min, hour, date, month, year;
//
// SET TIME ONCE (sec, min, hour, day, date, month, year)
// Example: Sunday = 1, date = 5 April 2026, time = 20:40:00
// DS1307_SetTime(0, 40, 20, 1, 5, 4, 26);
//
// while (1)
// {
//     DS1307_ReadTime(&sec, &min, &hour, &date, &month, &year);
//
//     LCD_SetCursor(0, 0);
//     snprintf(buffer, 17, "TIME: %02d:%02d:%02d", hour, min, sec);
//     SendLCDString(buffer);

//     LCD_SetCursor(1, 0);
//     snprintf(buffer, 17, "DATE: %02d/%02d/%02d", date, month, year);
//     SendLCDString(buffer);
//
//     _delay_ms(500);
// }

// Read Time (have day of week) ------------------
// uint8_t sec, min, hour, day, date, month, year;
// DS1307_ReadTime(&sec, &min, &hour, &day, &date, &month, &year);

// Convert day number to string
// char* GetDayName(uint8_t day)
// {
//     switch(day)
//     {
//         case 1: return "SUN";
//         case 2: return "MON";
//         case 3: return "TUE";
//         case 4: return "WED";
//         case 5: return "THU";
//         case 6: return "FRI";
//         case 7: return "SAT";
//         default: return "???";
//     }
// }
//
// snprintf(buffer, 17, "DAY: %s", GetDayName(day));
// SendLCDString(buffer);

// ==========================================================
// ALL FOR ONE Setup
// ==========================================================

int main(void)
{
    // All init functions
    delay_init();
    LCD_init();

    keypad_init();
    ultrasonic_init();

    SPI_Init();
    I2C_Init();

    sei(); // Enable global interrupts

    while (1)
    {
        /* code */
    }
}