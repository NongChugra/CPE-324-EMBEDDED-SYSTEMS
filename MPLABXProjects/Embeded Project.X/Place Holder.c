#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>

// ================= I2C (TWI) =================
#define SCL_CLOCK 100000UL

void I2C_Init(void)
{
    TWSR = 0x00; // prescaler = 1
    TWBR = ((F_CPU / SCL_CLOCK) - 16) / 2;
    TWCR = (1 << TWEN);
}

void I2C_Start(void)
{
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

void I2C_Stop(void)
{
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    _delay_us(20);
}

void I2C_Write(uint8_t data)
{
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

// ================= LCD I2C =================
// Change if your address is different
#define LCD_ADDR 0x27

// PCF8574 pin map (from your PDF)
#define LCD_RS  0x01   // P0
#define LCD_RW  0x02   // P1
#define LCD_EN  0x04   // P2
#define LCD_BL  0x08   // P3

uint8_t lcd_backlight = LCD_BL;

// Send 1 byte to PCF8574
void LCD_I2C_Write(uint8_t data)
{
    I2C_Start();
    I2C_Write((LCD_ADDR << 1) | 0); // write mode
    I2C_Write(data | lcd_backlight);
    I2C_Stop();
}

// pulse EN pin
void LCD_Pulse(uint8_t data)
{
    LCD_I2C_Write(data | LCD_EN);
    _delay_us(1);

    LCD_I2C_Write(data & ~LCD_EN);
    _delay_us(50);
}

// Send 4-bit (high nibble in bits P4-P7)
void LCD_Send4(uint8_t nibble, uint8_t mode)
{
    uint8_t data = (nibble & 0xF0) | mode;
    LCD_I2C_Write(data);
    LCD_Pulse(data);
}

void LCD_Command(uint8_t cmd)
{
    LCD_Send4(cmd & 0xF0, 0);
    LCD_Send4((cmd << 4) & 0xF0, 0);
}

void LCD_Data(uint8_t data)
{
    LCD_Send4(data & 0xF0, LCD_RS);
    LCD_Send4((data << 4) & 0xF0, LCD_RS);
}

void LCD_Clear(void)
{
    LCD_Command(0x01);
    _delay_ms(2);
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t addr = (row == 0) ? (0x80 + col) : (0xC0 + col);
    LCD_Command(addr);
}

void LCD_Print(const char *str)
{
    while (*str)
        LCD_Data(*str++);
}

// ================= LCD INIT =================
void LCD_Init(void)
{
    _delay_ms(50);

    // reset sequence (8-bit mode)
    LCD_Send4(0x30, 0);
    _delay_ms(5);

    LCD_Send4(0x30, 0);
    _delay_us(150);

    LCD_Send4(0x30, 0);
    _delay_ms(1);

    // set 4-bit mode
    LCD_Send4(0x20, 0);
    _delay_ms(1);

    // Function set: 4-bit, 2 line, 5x8
    LCD_Command(0x28);

    // Display ON, cursor OFF, blink OFF
    LCD_Command(0x0C);

    // Entry mode set: increment cursor
    LCD_Command(0x06);

    LCD_Clear();
}

// ================= MAIN =================
int main(void)
{
    I2C_Init();
    LCD_Init();

    LCD_SetCursor(0, 0);
    LCD_Print("Hello World!");

    LCD_SetCursor(1, 0);
    LCD_Print("ATmega328P");

    while (1)
    {
        // loop
    }
}