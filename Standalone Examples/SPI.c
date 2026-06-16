#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

volatile uint8_t spiByteCount = 0;
volatile uint8_t spiHighByte = 0;
volatile uint8_t spiLowByte = 0;
volatile uint16_t adcValue = 0;
volatile uint8_t adcReady = 0;

// =====================================
// SPI INIT
// =====================================

void SPI_Init()
{
    // MOSI, SCK, SS output
    DDRB |= (1 << PB3) | (1 << PB5) | (1 << PB2);

    // MISO input
    DDRB &= ~(1 << PB4);

    // SS high (inactive)
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

    // SS low
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

        // SS high (end)
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
    TCCR1A |= (0 << WGM11) | (0 << WGM10);
    TCCR1B |= (1 << WGM12);

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