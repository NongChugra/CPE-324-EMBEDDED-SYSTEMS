#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#define LCD_DATA_PORT PORTD
#define LCD_DATA_DDR DDRD

#define LCD_E_PORT PORTD
#define LCD_E_DDR DDRD
#define LCD_E_PIN PD0

#define LCD_RS_PORT PORTD
#define LCD_RS_DDR DDRD
#define LCD_RS_PIN PD1

// =====================================
// GLOBALS
// =====================================
volatile uint8_t spiByteCount = 0;
volatile uint8_t spiHighByte = 0;
volatile uint8_t spiLowByte = 0;
volatile uint16_t adcValue = 0;
volatile uint8_t adcReady = 0;

// =====================================
// LCD FUNCTIONS
// =====================================

void commitData()
{
    LCD_E_PORT |= (1 << LCD_E_PIN);
    _delay_us(1);
    LCD_E_PORT &= ~(1 << LCD_E_PIN);
    _delay_us(100);
}

void SendLCDCommand(uint8_t command)
{
    // RS = 0
    LCD_RS_PORT &= ~(1 << LCD_RS_PIN);

    // high nibble
    LCD_DATA_PORT &= 0x0F;
    LCD_DATA_PORT |= (command & 0xF0);
    commitData();

    // low nibble
    LCD_DATA_PORT &= 0x0F;
    LCD_DATA_PORT |= ((command << 4) & 0xF0);
    commitData();

    _delay_ms(2);
}

void SendLCDData(uint8_t data)
{
    // RS = 1
    LCD_RS_PORT |= (1 << LCD_RS_PIN);

    // high nibble
    LCD_DATA_PORT &= 0x0F;
    LCD_DATA_PORT |= (data & 0xF0);
    commitData();

    // low nibble
    LCD_DATA_PORT &= 0x0F;
    LCD_DATA_PORT |= ((data << 4) & 0xF0);
    commitData();

    _delay_ms(2);
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

void initLCD()
{
    // PD4..PD7 output for data
    LCD_DATA_DDR |= 0xF0;
    LCD_DATA_PORT &= ~0xF0;

    // E and RS outputs
    LCD_E_DDR |= (1 << LCD_E_PIN);
    LCD_RS_DDR |= (1 << LCD_RS_PIN);

    LCD_E_PORT &= ~(1 << LCD_E_PIN);
    LCD_RS_PORT &= ~(1 << LCD_RS_PIN);

    _delay_ms(40);

    // init sequence
    SendLCDCommand(0x33);
    SendLCDCommand(0x32);

    SendLCDCommand(0x28);
    SendLCDCommand(0x0C);
    SendLCDCommand(0x01);
    SendLCDCommand(0x06);
    _delay_ms(2);
}

// =====================================
// SPI INIT
// =====================================

void SPI_Init()
{
    // MOSI, SCK, CS output
    DDRB |= (1 << PB3) | (1 << PB5) | (1 << PB2);

    // MISO input
    DDRB &= ~(1 << PB4);

    // CS high (inactive)
    PORTB |= (1 << PB2);

    // SPI Enable, Master, SPI Interrupt Enable
    // Clock = fosc/8
    SPCR = (1 << SPE) | (1 << MSTR) | (0 << SPR1) | (1 << SPR0) | (1 << SPIE);
    SPSR = (1 << SPI2X);
}

// Start reading MCP3201 (2 bytes)
void MCP3201_StartConversion()
{
    adcReady = 0;
    spiByteCount = 0;

    // CS low
    PORTB &= ~(1 << PB2);

    // start first transfer (dummy)
    SPDR = 0;
}

// SPI Interrupt: runs after each byte transfer complete
ISR(SPI_STC_vect)
{
    uint8_t received = SPDR;

    if (spiByteCount == 0)
    {
        spiHighByte = received;
        spiByteCount++;

        // start second transfer
        SPDR = 0;
    }
    else
    {
        spiLowByte = received;

        // CS high (end)
        PORTB |= (1 << PB2);

        // MCP3201 format extraction
        adcValue = (((uint16_t)(spiHighByte & 0x1F)) << 8 | spiLowByte) >> 1;
        adcReady = 1;
    }
}

// =====================================
// TIMER1
// =====================================
// Trigger ADC read every 500 ms

void Timer1_Init()
{
    TCCR1A = 0;
    TCCR1B = 0;

    // CTC mode (Clear Timer on Compare Match)
    TCCR1B |= (1 << WGM12) | (0 << WGM11) | (0 << WGM10);

    // prescaler = 256
    TCCR1B |= (1 << CS12) | (0 << CS11) | (0 << CS10);

    // 8000000 Hz / 256 = 31250 Hz
    OCR1A = 15624;

    // enable compare interrupt
    TIMSK1 |= (1 << OCIE1A);
}

ISR(TIMER1_COMPA_vect)
{
    MCP3201_StartConversion();
}

// =====================================
// MAIN
// =====================================

int main(void)
{
    initLCD();
    SPI_Init();
    Timer1_Init();

    sei();

    char line[17];

    SendLCDCommand(0x01);
    LCD_SetCursor(0, 0);
    SendLCDString("Temp Sensor");

    while (1)
    {
        if (adcReady)
        {
            adcReady = 0;

            // Convert ADC -> voltage -> temperature
            float voltage = (adcValue * 5.0f) / 4096.0f;
            float tempC = (voltage - 0.5f) / 0.01f;

            // display
            LCD_SetCursor(1, 0);
            snprintf(line, 16, "T = %.2f C      ", tempC);
            SendLCDString(line);
        }
    }
}