#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#define LCD_DATA_PORT   PORTD
#define LCD_DATA_DDR    DDRD

#define LCD_E_PORT      PORTD
#define LCD_E_DDR       DDRD
#define LCD_E_PIN       PD0

#define LCD_RS_PORT     PORTD
#define LCD_RS_DDR      DDRD
#define LCD_RS_PIN      PD1

// ==========================
// LCD FUNCTIONS
// ==========================
void commitData()
{
    LCD_E_PORT |= (1 << LCD_E_PIN);
    _delay_us(1);
    LCD_E_PORT &= ~(1 << LCD_E_PIN);
    _delay_us(100);
}

void SendLCDCommand(uint8_t command)
{
    LCD_RS_PORT &= ~(1 << LCD_RS_PIN);

    LCD_DATA_PORT &= 0x0F;
    LCD_DATA_PORT |= (command & 0xF0);
    commitData();

    LCD_DATA_PORT &= 0x0F;
    LCD_DATA_PORT |= ((command << 4) & 0xF0);
    commitData();

    _delay_ms(2);
}

void SendLCDData(uint8_t data)
{
    LCD_RS_PORT |= (1 << LCD_RS_PIN);

    LCD_DATA_PORT &= 0x0F;
    LCD_DATA_PORT |= (data & 0xF0);
    commitData();

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
    LCD_DATA_DDR |= 0xF0;
    LCD_DATA_PORT &= ~0xF0;

    LCD_E_DDR |= (1 << LCD_E_PIN);
    LCD_RS_DDR |= (1 << LCD_RS_PIN);

    LCD_E_PORT &= ~(1 << LCD_E_PIN);
    LCD_RS_PORT &= ~(1 << LCD_RS_PIN);

    _delay_ms(40);

    SendLCDCommand(0x33);
    SendLCDCommand(0x32);

    SendLCDCommand(0x28);
    SendLCDCommand(0x0C);
    SendLCDCommand(0x01);
    SendLCDCommand(0x06);

    _delay_ms(2);
}

// ==========================
// I2C (TWI) FUNCTIONS
// ==========================
#define DS1307_ADDRESS 0x68 // The 7-bit I2C address of the DS1307

void I2C_Init()
{
    // SCL = F_CPU / (16 + 2 * TWBR * prescaler)
    // TWBR = 32, prescaler = 1
    TWBR = 32;
    TWSR |= (0 << TWPS1) | (0 << TWPS0);
}

void I2C_Start()
{
    // Interrupt, Enable, Start
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTA);
    while (!(TWCR & (1 << TWINT)));
}

void I2C_Stop()
{
    // Interrupt, Enable, Stop
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
    _delay_us(10);
}

void I2C_Write(uint8_t data)
{
    // Interrupt, Enable
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

uint8_t I2C_Read_ACK()
{
    // Interrupt, Enable, Enable Acknowledge
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

uint8_t I2C_Read_NACK()
{
    // Interrupt, Enable
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

// ==========================
// DS1307 FUNCTIONS
// ==========================

// Convert BCD to decimal
uint8_t BCD_to_DEC(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// Read time/date from DS1307
void DS1307_ReadTime(uint8_t *sec, uint8_t *min, uint8_t *hour,
                     uint8_t *day, uint8_t *date, uint8_t *month, uint8_t *year)
{
    I2C_Start();
    I2C_Write((DS1307_ADDRESS << 1) | 0); // write mode
    I2C_Write(0); // start at register 0
    I2C_Stop();

    I2C_Start();
    I2C_Write((DS1307_ADDRESS << 1) | 1); // read mode

    *sec   = BCD_to_DEC(I2C_Read_ACK() & 0x7F);
    *min   = BCD_to_DEC(I2C_Read_ACK());
    *hour  = BCD_to_DEC(I2C_Read_ACK() & 0x3F);
    *day   = BCD_to_DEC(I2C_Read_ACK());
    *date  = BCD_to_DEC(I2C_Read_ACK());
    *month = BCD_to_DEC(I2C_Read_ACK());
    *year  = BCD_to_DEC(I2C_Read_NACK());

    I2C_Stop();
}

int main(void)
{
    char buffer[17];

    uint8_t sec, min, hour;
    uint8_t day, date, month, year;

    initLCD();
    I2C_Init();

    while (1)
    {
        DS1307_ReadTime(&sec, &min, &hour, &day, &date, &month, &year);

        // Line 1: TIME
        LCD_SetCursor(0, 0);
        snprintf(buffer, 17, "TIME: %02d:%02d:%02d", hour, min, sec);
        SendLCDString(buffer);

        // Line 2: DATE
        LCD_SetCursor(1, 0);
        snprintf(buffer, 17, "DATE: %02d/%02d/%02d", date, month, year);
        SendLCDString(buffer);

        _delay_ms(500);
    }
}