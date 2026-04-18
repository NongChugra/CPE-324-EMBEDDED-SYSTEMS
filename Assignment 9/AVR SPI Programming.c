#define F_CPU 8000000UL //8MHz

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdint.h>

// --- LCD Pin Definitions (PC0-PC3 Data, PB0-PB1 Control) ---
#define LCD_DATA_PORT PORTC
#define LCD_DATA_DDR DDRC
#define LCD_CTRL_PORT PORTB
#define LCD_CTRL_DDR DDRB

#define LCD_E_PIN PB1
#define LCD_RS_PIN PB0

#define DS1307_ADDRESS 0x68

// ==========================
// LCD FUNCTIONS
// ==========================
void commitData() {
    LCD_CTRL_PORT |= (1 << LCD_E_PIN);
    _delay_us(1);
    LCD_CTRL_PORT &= ~(1 << LCD_E_PIN);
    _delay_us(100);
}

void SendLCDCommand(uint8_t command) {
    LCD_CTRL_PORT &= ~(1 << LCD_RS_PIN);
    LCD_DATA_PORT = (LCD_DATA_PORT & 0xF0) | (command >> 4);
    commitData();
    LCD_DATA_PORT = (LCD_DATA_PORT & 0xF0) | (command & 0x0F);
    commitData();
    _delay_ms(2);
}

void SendLCDData(uint8_t data) {
    LCD_CTRL_PORT |= (1 << LCD_RS_PIN);
    LCD_DATA_PORT = (LCD_DATA_PORT & 0xF0) | (data >> 4);
    commitData();
    LCD_DATA_PORT = (LCD_DATA_PORT & 0xF0) | (data & 0x0F);
    commitData();
    _delay_ms(2);
}

void SendLCDString(const char *str) {
    while (*str) SendLCDData(*str++);
}

void LCD_SetCursor(uint8_t row, uint8_t col) {
    uint8_t address = (row == 0) ? (0x80 + col) : (0xC0 + col);
    SendLCDCommand(address);
}

void initLCD() {
    LCD_DATA_DDR |= 0x0F;
    
    LCD_CTRL_DDR |= (1 << LCD_E_PIN) | (1 << LCD_RS_PIN);
    
    _delay_ms(50);
    SendLCDCommand(0x33);
    SendLCDCommand(0x32);
    SendLCDCommand(0x28);
    SendLCDCommand(0x0C);
    SendLCDCommand(0x01);
    SendLCDCommand(0x06);
}

// ==========================
// I2C FUNCTIONS
// ==========================
void I2C_Init() {
    TWBR = 32;
    TWSR = 0;
    // SCL Freq = F_CPU / (16+2(TWBR)+Prescaler)
    // SCL Freq = 100kHz
}

void I2C_Start() {
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTA);
    while (!(TWCR & (1 << TWINT)));
}

void I2C_Stop() {
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
    _delay_us(100);
}

void I2C_Write(uint8_t data) {
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

uint8_t I2C_Read_ACK() {
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

uint8_t I2C_Read_NACK() {
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

// ==========================
// RTC HELPERS
// ==========================
uint8_t BCD_to_DEC(uint8_t bcd) { return ((bcd >> 4) * 10) + (bcd & 0x0F); }
uint8_t DEC_to_BCD(uint8_t dec) { return ((dec / 10) << 4) | (dec % 10); }

void DS1307_SetTime(uint8_t s, uint8_t m, uint8_t h, uint8_t d, uint8_t dt, uint8_t mt, uint8_t y) {
    I2C_Start();
    I2C_Write(DS1307_ADDRESS << 1);
    I2C_Write(0x00);
    I2C_Write(DEC_to_BCD(s)); // Seconds (Starts Oscillator)
    I2C_Write(DEC_to_BCD(m)); // Minutes
    I2C_Write(DEC_to_BCD(h)); // Hours
    I2C_Write(DEC_to_BCD(d)); // Day of week
    I2C_Write(DEC_to_BCD(dt)); // Date
    I2C_Write(DEC_to_BCD(mt)); // Month
    I2C_Write(DEC_to_BCD(y)); // Year
    I2C_Stop();
}

void DS1307_ReadTime(uint8_t *sec, uint8_t *min, uint8_t *hour, uint8_t *date, uint8_t *month, uint8_t *year) {
    I2C_Start();
    I2C_Write(DS1307_ADDRESS << 1);
    I2C_Write(0x00);
    I2C_Start(); // Repeated Start
    I2C_Write((DS1307_ADDRESS << 1) | 1);
    *sec = BCD_to_DEC(I2C_Read_ACK() & 0x7F);
    *min = BCD_to_DEC(I2C_Read_ACK());
    *hour = BCD_to_DEC(I2C_Read_ACK() & 0x3F);
    I2C_Read_ACK(); // Skip Day
    *date = BCD_to_DEC(I2C_Read_ACK());
    *month = BCD_to_DEC(I2C_Read_ACK());
    *year = BCD_to_DEC(I2C_Read_NACK());
    I2C_Stop();
}

// ==========================
// MAIN
// ==========================
int main(void) {
    char buffer[17];
    uint8_t sec, min, hour, date, month, year;
    
    initLCD();
    I2C_Init();

    // SET THE TIME
    //(sec, min, hour, day_of_week, date, month, year)
    // Sunday = 1. Year = 26.
    DS1307_SetTime(0, 40, 20, 1, 5, 4, 26);

    while (1) {
        DS1307_ReadTime(&sec, &min, &hour, &date, &month, &year);
        
        LCD_SetCursor(0, 0);
        snprintf(buffer, 17, "TIME: %02d:%02d:%02d", hour, min, sec);
        SendLCDString(buffer);
        
        LCD_SetCursor(1, 0);
        snprintf(buffer, 17, "DATE: %02d/%02d/%02d", date, month, year);
        SendLCDString(buffer);
        
        _delay_ms(500);
    }
}