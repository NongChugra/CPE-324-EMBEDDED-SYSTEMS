#define F_CPU 8000000UL

#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// === Delay ==================================================================

// void Delay_Init()
//{
//     TCCR0A = 0;
//     TCCR0B = 0;
//     TCNT0 = 0; // Timer/Counter Register
//
//     // CTC Mode (WGM01 = 1)
//     TCCR0A |= (1 << WGM01);
//
//     // Prescaler = 8  (Tick = 1us at 8MHz)
//     TCCR0B |= (1 << CS01);
// }
//
// void my_delay_us(uint16_t us)
//{
//     while (us > 0)
//     {
//         uint8_t chunk;
//
//         if (us > 255)
//             chunk = 255;
//         else
//             chunk = us;
//
//         TCNT0 = 0; // Timer/Counter Register
//         OCR0A = chunk - 1;
//
//         TIFR0 |= (1 << OCF0A); // Clear flag
//         while (!(TIFR0 & (1 << OCF0A)))
//             ;
//         TIFR0 |= (1 << OCF0A); // Clear flag
//
//         us -= chunk;
//     }
// }
//
// void my_delay_ms(uint16_t ms)
//{
//     while (ms--)
//     {
//         my_delay_us(1000);
//     }
// }

// === I2C ====================================================================

void I2C_Init(void)
{
    // SCL = F_CPU / (16 + 2 * TWBR * Prescaler)
    // F_CPU = 8MHz, TWBR = 32, Prescaler = 1
    // SCL = 8MHz / (16 + 2 * 32) = 8MHz / 80 = 100kHz
    TWBR = 32;   // Bit rate register
    TWSR = 0x00; // Prescaler = 1

    TWCR = (1 << TWEN); // Enable I2C
}

void I2C_Start(void)
{
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)))
        ;
}

void I2C_Stop(void)
{
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    // Wait a little to complete stop condition
    _delay_us(20);
}

void I2C_Write(uint8_t data)
{
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)))
        ;
}

// === LCD I2C ================================================================

#define LCD_I2C_ADDR 0x27

// PCF8574 pin mapping
#define LCD_RS (1 << 0)
#define LCD_EN (1 << 2)

void LCD_I2C_Write(uint8_t data)
{
    data &= ~(1 << 1); // RW = 0
    data |= (1 << 3);  // BL = 1

    I2C_Start();
    I2C_Write(LCD_I2C_ADDR << 1); // SLA + W
    I2C_Write(data);
    I2C_Stop();
}

void LCD_I2C_commitData(uint8_t data)
{
    LCD_I2C_Write(data | LCD_EN);
    _delay_us(1);

    LCD_I2C_Write(data & ~LCD_EN);
    _delay_us(50);
}

// Send 4-bit nibble (nibble in low 4 bits, mapped to P4-P7)
void LCD_I2C_SendNibble(uint8_t nibble, uint8_t rs)
{
    uint8_t data = (nibble & 0x0F) << 4;

    if (rs)
        data |= LCD_RS;

    LCD_I2C_commitData(data);
}

// Send full byte (high nibble then low nibble)
void LCD_I2C_SendByte(uint8_t byte, uint8_t rs)
{
    LCD_I2C_SendNibble(byte >> 4, rs);
    LCD_I2C_SendNibble(byte & 0x0F, rs);
}

void LCD_I2C_Command(uint8_t cmd)
{
    LCD_I2C_SendByte(cmd, 0);
}

void LCD_I2C_Data(uint8_t data)
{
    LCD_I2C_SendByte(data, 1);
}

void LCD_I2C_String(const char *str)
{
    while (*str)
        LCD_I2C_Data(*str++);
}

void LCD_I2C_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t address = (row == 0) ? (0x80 + col) : (0xC0 + col);
    LCD_I2C_Command(address);
}

void LCD_I2C_Clear(void)
{
    LCD_I2C_Command(0x01);
    _delay_ms(2);
}

void LCD_I2C_Init(void)
{
    _delay_ms(50);

    // HD44780 requires sending 0x03 three times to reliably
    // reset from any unknown state before switching to 4-bit mode.
    LCD_I2C_SendNibble(0x03, 0);
    _delay_ms(5);

    LCD_I2C_SendNibble(0x03, 0);
    _delay_us(150);

    LCD_I2C_SendNibble(0x03, 0);
    _delay_us(150);

    LCD_I2C_SendNibble(0x02, 0); // switch to 4-bit mode
    _delay_us(150);

    LCD_I2C_Command(0x28); // Function set : 4-bit | 2-line | 5?8 font
    LCD_I2C_Command(0x0C); // Display on, cursor off, blink off
    LCD_I2C_Command(0x01); // Clear display
    _delay_ms(2);
    LCD_I2C_Command(0x06); // Entry mode : increment, no shift
}

// === Rotary Encoder Switch ==================================================

// Rotary encoder pins
#define ENC_A PD5  // PCINT21
#define ENC_B PD7  // PCINT23
#define ENC_SW PD4 // PCINT20

typedef enum
{
    STATE_PAUSE = 0,
    STATE_PLAY = 1
} PlayState;

volatile uint8_t bpm = 120;        // 60-240
volatile int8_t key_transpose = 0; // -12 to +12
volatile PlayState state = STATE_PAUSE;

volatile uint8_t lcd_update_flag = 1; // request LCD update

void Rotary_Encoder_Init(void)
{
    // PD4, PD5, PD7 as input with pull-up
    DDRD &= ~((1 << ENC_A) | (1 << ENC_B) | (1 << ENC_SW));
    PORTD |= ((1 << ENC_A) | (1 << ENC_B) | (1 << ENC_SW));

    // Unmask PCINT20 (PD4), PCINT21 (PD5), PCINT23 (PD7)
    PCMSK2 |= (1 << PCINT20) | (1 << PCINT21) | (1 << PCINT23);

    // Enable Pin Change Interrupt group 2
    PCICR |= (1 << PCIE2);
}

static const char *note_names[] =
    {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

static const int8_t note_row_base_idx[] = {0, 23, 21, 19, 17, 16, 14, 12};

int8_t NoteRow_ToIndex(uint8_t y)
{
    if (y < 1 || y > 7)
        return -1;

    int8_t idx = note_row_base_idx[y] + key_transpose;

    if (idx < 0)
        idx = 0;
    if (idx > 35)
        idx = 35;

    return idx;
}

void LCD_Update_Status(uint8_t cursor_y)
{
    char buf[17];
    char note_label[4];

    LCD_I2C_SetCursor(0, 0);
    if (state == STATE_PLAY)
        LCD_I2C_String("[PLAY] ");
    else
        LCD_I2C_String("[PAUSE]");

    LCD_I2C_SetCursor(0, 8);
    sprintf(buf, "BPM:%3d", (int)bpm);
    LCD_I2C_String(buf);

    int8_t note_idx = NoteRow_ToIndex(cursor_y);
    uint8_t note = (uint8_t)note_idx % 12;
    uint8_t octave = 3 + ((uint8_t)note_idx / 12);

    sprintf(note_label, "%s%d", note_names[note], octave);

    LCD_I2C_SetCursor(1, 0);
    sprintf(
        buf,
        "KEY:%-3s (%+d)   ",
        note_label,
        (int)key_transpose);

    buf[16] = '\0';
    LCD_I2C_String(buf);
}

ISR(PCINT2_vect)
{
    static uint8_t last_pind = 0xFF; // pull-ups
    static uint8_t last_a = 1;
    static uint8_t sw_lock = 0; // re-trigger guard

    uint8_t curr_pind = PIND;
    uint8_t changed = last_pind ^ curr_pind; // check which pins changed

    // Push button (ENC_SW / PD4)
    if (changed & (1 << ENC_SW))
    {
        uint8_t sw_now = (curr_pind & (1 << ENC_SW)) ? 1 : 0;

        // Act on falling edge (button press, active low)
        if (sw_now == 0 && sw_lock == 0)
        {
            state = (state == STATE_PLAY) ? STATE_PAUSE : STATE_PLAY;
            sw_lock = 1;
            lcd_update_flag = 1;
        }

        // Clear sw_lock on release (rising edge)
        if (sw_now == 1)
            sw_lock = 0;
    }

    // Rotary encoder (ENC_A / PD5) (ENC_B / PD7)
    if (changed & (1 << ENC_A))
    {
        uint8_t a = (curr_pind & (1 << ENC_A)) ? 1 : 0;
        uint8_t b = (curr_pind & (1 << ENC_B)) ? 1 : 0;

        // Act on ENC_A falling edge only
        if (last_a == 1 && a == 0)
        {
            if (b)
            {
                if (bpm <= 235)
                    bpm += 5;
            }
            else
            {
                if (bpm >= 65)
                    bpm -= 5;
            }

            lcd_update_flag = 1;
        }

        last_a = a;
    }

    last_pind = curr_pind;
}

// === Passive Buzzer Modules (Hardware PWM) ==================================

void Buzzer1_Init(void)
{
    DDRB |= (1 << PB1);

    // Clear on Compare, OC1A non-inverting, Fast PWM, TOP = ICR1
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12);
}

void Buzzer1_Stop(void)
{
    // stop timer clock
    TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));

    // make pin as input
    DDRB &= ~(1 << PB1);
    PORTB &= ~(1 << PB1);
}

void Buzzer1_Play(uint16_t freq)
{
    if (!freq)
    {
        Buzzer1_Stop();
        return;
    }
    // enable pin output
    DDRB |= (1 << PB1);

    uint16_t top = (uint16_t)((uint32_t)F_CPU / (8UL * freq)) - 1;
    ICR1 = top;
    OCR1A = top >> 1; // 50% Duty Cycle

    TCCR1B |= (1 << CS11); // Start timer (Prescaler 8)
}

typedef enum
{
    T2_PS_1 = (1 << CS20),                               // ÷1
    T2_PS_8 = (1 << CS21),                               // ÷8
    T2_PS_32 = (1 << CS21) | (1 << CS20),                // ÷32
    T2_PS_64 = (1 << CS22),                              // ÷64
    T2_PS_128 = (1 << CS22) | (1 << CS20),               // ÷128
    T2_PS_256 = (1 << CS22) | (1 << CS21),               // ÷256
    T2_PS_1024 = (1 << CS22) | (1 << CS21) | (1 << CS20) // ÷1024
} T2Prescaler;

static const uint16_t t2_divisors[] = {1, 8, 32, 64, 128, 256, 1024};
static const uint8_t t2_ps_bits[] = {T2_PS_1, T2_PS_8, T2_PS_32, T2_PS_64,
                                     T2_PS_128, T2_PS_256, T2_PS_1024};

void Buzzer2_Init(void)
{
    DDRB |= (1 << PD3);

    // Clear on Compare, OC2B non-inverting, Fast PWM, TOP = OCR2B
    TCCR2A = (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
    TCCR2B = (1 << WGM22);
}

void Buzzer2_Stop(void)
{
    TCCR2B = (1 << WGM22); // This also set CS = 000

    DDRD &= ~(1 << PD3);
    PORTD &= ~(1 << PD3);
}

void Buzzer2_Play(uint16_t freq)
{
    if (!freq)
    {
        Buzzer2_Stop();
        return;
    }
    DDRD |= (1 << PD3);

    // Fallback, Safe Default
    uint8_t best_top = 255;
    uint8_t best_ps = T2_PS_1024;

    // Walk prescalers from largest to smallest
    // Pick the last one whose TOP fits in 8 bits (0-255)
    // Smallest valid prescaler = best resolution
    for (int8_t i = 6; i >= 0; i--)
    {
        uint32_t top = (uint32_t)F_CPU / ((uint32_t)t2_divisors[i] * freq);

        if (top == 0)
            top = 1; // avoid division by zero for very high frequencies
        top -= 1;    // TOP = (F_CPU / (ps * freq)) - 1

        if (top <= 255)
        {
            best_top = (uint8_t)top;
            best_ps = t2_ps_bits[i];
            // keep going to find smallest valid prescaler
        }
    }

    OCR2A = best_top;
    OCR2B = best_top >> 1; // 50% Duty Cycle

    TCCR2B |= best_ps; // Start timer with selected prescaler
}

typedef enum
{
    T0_PS_1 = (1 << CS00),                 // ÷1
    T0_PS_8 = (1 << CS01),                 // ÷8
    T0_PS_64 = (1 << CS01) | (1 << CS00),  // ÷64
    T0_PS_256 = (1 << CS02),               // ÷256
    T0_PS_1024 = (1 << CS02) | (1 << CS00) // ÷1024
} T0Prescaler;

static const uint16_t t0_divisors[] = {1, 8, 64, 256, 1024};
static const uint8_t t0_ps_bits[] = {T0_PS_1, T0_PS_8, T0_PS_64, T0_PS_256, T0_PS_1024};

void Buzzer3_Init(void)
{
    DDRD |= (1 << PD6);

    // Clear on Compare, OC0A non-inverting, Fast PWM, TOP = OCR0A
    TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);
    TCCR0B = (1 << WGM02);
}

void Buzzer3_Stop(void)
{
    TCCR0B = (1 << WGM02); // This also set CS = 000

    DDRD &= ~(1 << PD6);
    PORTD &= ~(1 << PD6);
}

void Buzzer3_Play(uint16_t freq)
{
    if (!freq)
    {
        Buzzer3_Stop();
        return;
    }
    DDRD |= (1 << PD6);

    uint8_t best_top = 255;
    uint8_t best_ps = T0_PS_1024;

    for (int8_t i = 4; i >= 0; i--)
    {
        uint32_t top = (uint32_t)F_CPU / ((uint32_t)t0_divisors[i] * freq);

        if (top == 0)
            top = 1;
        top -= 1;

        if (top <= 255)
        {
            best_top = (uint8_t)top;
            best_ps = t0_ps_bits[i];
        }
    }

    OCR0A = best_top;
    OCR0B = best_top >> 1;

    TCCR0B = (1 << WGM02) | best_ps;
}

void All_Stop(void)
{
    Buzzer1_Stop();
    Buzzer2_Stop();
    Buzzer3_Stop();
}

// === MAX7219 32x8 LED Matrix ================================================

#define MATRIX_NUM_CHIPS 4
#define MATRIX_CS PB2

// MAX7219 register addresses
#define MAX7219_NOOP 0x00
#define MAX7219_DIGIT(n) (0x01 + (n)) // n = 0-7 (row 0-7)
#define MAX7219_DECODE 0x09
#define MAX7219_INTENSITY 0x0A
#define MAX7219_SCANLIMIT 0x0B
#define MAX7219_SHUTDOWN 0x0C
#define MAX7219_DISPLAYTEST 0x0F

// Shadow buffer: [chip][row], each byte = 8 column bits for that row
static uint8_t matrix_buf[MATRIX_NUM_CHIPS][8];

void SPI_Init(void)
{
    // MOSI, SCK, CS as output - MISO didn't use
    DDRB |= (1 << PB3) | (1 << PB5) | (1 << PB2);

    // CS idle high
    PORTB |= (1 << PB2);

    // SPI enable, Master, fosc/2 (~4MHz)
    SPCR = (1 << SPE) | (1 << MSTR);
    SPSR = (1 << SPI2X);
}

static void SPI_WriteByte(uint8_t data)
{
    SPDR = data;
    while (!(SPSR & (1 << SPIF)))
        ; // wait for transfer complete
}

// Send one register + data pair to every chip in the chain.
static void MAX7219_SendAll(uint8_t reg, uint8_t data)
{
    PORTB &= ~(1 << MATRIX_CS); // CS low

    for (uint8_t i = 0; i < MATRIX_NUM_CHIPS; i++)
    {
        SPI_WriteByte(reg);
        SPI_WriteByte(data);
    }

    PORTB |= (1 << MATRIX_CS);
}

void Matrix_Clear(void)
{
    for (uint8_t c = 0; c < MATRIX_NUM_CHIPS; c++)
        for (uint8_t r = 0; r < 8; r++)
            matrix_buf[c][r] = 0x00;

    for (uint8_t row = 0; row < 8; row++)
        MAX7219_SendAll(MAX7219_DIGIT(row), 0x00);
}

void Matrix_Init(void)
{
    SPI_Init();

    // config
    MAX7219_SendAll(MAX7219_DISPLAYTEST, 0x00); // display test off
    MAX7219_SendAll(MAX7219_DECODE, 0x00);      // no BCD decode (raw bits)
    MAX7219_SendAll(MAX7219_SCANLIMIT, 0x07);   // scan all 8 rows
    MAX7219_SendAll(MAX7219_INTENSITY, 0x0F);   // brightness mid (0?15)
    MAX7219_SendAll(MAX7219_SHUTDOWN, 0x01);    // normal operation

    // clear display
    Matrix_Clear();
}

void Matrix_SetBrightness(uint8_t level)
{
    if (level > 15)
        level = 15;
    MAX7219_SendAll(MAX7219_INTENSITY, level);
}

void Matrix_ToggleLED(uint8_t x, uint8_t y)
{
    if (x >= 32 || y >= 8)
        return; // bounds check

    uint8_t chip = x / 8; // (0-3)
    uint8_t col = x % 8;  // (0-7)

    // Toggle bit in shadow buffer (bit 7 = leftmost column of chip)
    matrix_buf[chip][y] ^= (1 << (7 - col));

    // update only this row on the correct chip
    PORTB &= ~(1 << MATRIX_CS);

    for (uint8_t i = 0; i < MATRIX_NUM_CHIPS; i++)
    {
        if (i == chip)
        {
            SPI_WriteByte(MAX7219_DIGIT(y));
            SPI_WriteByte(matrix_buf[chip][y]);
        }
        else
        {
            SPI_WriteByte(MAX7219_NOOP);
            SPI_WriteByte(0x00);
        }
    }

    PORTB |= (1 << MATRIX_CS);
}

// === Key Pad ================================================================

#define KEYPAD_ROW_DDR DDRD
#define KEYPAD_ROW_PORT PORTD

#define KEYPAD_EXTRA_ROW_DDR DDRC
#define KEYPAD_EXTRA_ROW_PORT PORTC

#define KEYPAD_COL_DDR DDRC
#define KEYPAD_COL_PORT PORTC
#define KEYPAD_COL_PIN PINC

#define ROW_MASK ((1 << PD0) | (1 << PD1) | (1 << PD2))
#define EXTRA_ROW_PIN PC3
#define COL_MASK ((1 << PC0) | (1 << PC1) | (1 << PC2))

char keymap[4][3] =
    {
        {'9', '8', '7'},
        {'6', '5', '4'},
        {'3', '2', '1'},
        {'3', '2', '1'}
};

void Keypad_AllRowsHigh(void)
{
    KEYPAD_ROW_PORT |= ROW_MASK;
    KEYPAD_EXTRA_ROW_PORT |= (1 << EXTRA_ROW_PIN);
}

void Keypad_SetRowLow(uint8_t row)
{
    Keypad_AllRowsHigh();

    if (row < 3)
        KEYPAD_ROW_PORT &= ~(1 << (PD0 + row));
    else
        KEYPAD_EXTRA_ROW_PORT &= ~(1 << EXTRA_ROW_PIN);
}

void Keypad_Init(void)
{
    // ROW 0-2 = OUTPUT on PD0, PD1, PD2
    KEYPAD_ROW_DDR |= ROW_MASK;
    KEYPAD_ROW_PORT |= ROW_MASK;

    // ROW 3 = OUTPUT on PC3
    KEYPAD_EXTRA_ROW_DDR |= (1 << EXTRA_ROW_PIN);
    KEYPAD_EXTRA_ROW_PORT |= (1 << EXTRA_ROW_PIN);

    // COL = INPUT on PC0, PC1, PC2
    KEYPAD_COL_DDR &= ~COL_MASK;

    // Pull-up enable on COL
    KEYPAD_COL_PORT |= COL_MASK;
}

char Keypad_Read(void)
{
    uint8_t oldSREG = SREG;
    cli();

    for (uint8_t row = 0; row < 4; row++)
    {
        Keypad_SetRowLow(row);
        _delay_us(5);

        uint8_t col_state = KEYPAD_COL_PIN & COL_MASK;

        Keypad_AllRowsHigh();

        if (col_state != COL_MASK)
        {
            for (uint8_t col = 0; col < 3; col++)
            {
                if (!(col_state & (1 << (PC0 + col))))
                {
                    SREG = oldSREG;
                    return keymap[row][col];
                }
            }
        }
    }

    SREG = oldSREG;
    return 0;
}

// === Sequencer / Note Buffer ===============================================

// Chromatic frequencies from C3 to B5
static const uint16_t CHROMATIC_FREQS[] = {
    131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247, // C3-B3 (0-11)
    262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, // C4-B4 (12-23)
    523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988  // C5-B5 (24-35)
};

static uint8_t note_buf[8][32];

uint16_t NoteRow_ToFreq(uint8_t y)
{
    int8_t idx = NoteRow_ToIndex(y);
    if (idx < 0)
        return 0;

    return CHROMATIC_FREQS[(uint8_t)idx];
}

void Note_Toggle(uint8_t x, uint8_t y)
{
    if (x >= 32 || y < 1 || y > 7)
        return;

    note_buf[y][x] ^= 1;
    Matrix_ToggleLED(x, y);
}

void Note_ClearAll(void)
{
    for (uint8_t y = 1; y < 8; y++)
    {
        for (uint8_t x = 0; x < 32; x++)
        {
            note_buf[y][x] = 0;
        }

        for (uint8_t c = 0; c < MATRIX_NUM_CHIPS; c++)
        {
            matrix_buf[c][y] = 0x00;
        }

        // Flush row to MAX7219
        PORTB &= ~(1 << MATRIX_CS);
        for (uint8_t c = 0; c < MATRIX_NUM_CHIPS; c++)
        {
            SPI_WriteByte(MAX7219_DIGIT(y));
            SPI_WriteByte(0x00);
        }
        PORTB |= (1 << MATRIX_CS);
    }
}

// Find the last column that contains a note
uint8_t Sequencer_GetLastActiveColumn(void)
{
    for (int8_t x = 31; x >= 0; x--)
    {
        for (uint8_t y = 1; y <= 7; y++)
        {
            if (note_buf[y][x])
                return x;
        }
    }
    return 0;
}

// Play notes at current step column
void Sequencer_PlayColumn(uint8_t step_x)
{
    uint16_t freqs[3] = {0, 0, 0};
    uint8_t count = 0;

    for (int8_t y = 7; y >= 1; y--)
    {
        if (note_buf[y][step_x])
        {
            freqs[count++] = NoteRow_ToFreq(y);
            if (count >= 3)
                break;
        }
    }

    Buzzer1_Play(freqs[0]);
    Buzzer2_Play(freqs[1]);
    Buzzer3_Play(freqs[2]);
}

// === Step Runner ============================================================

void StepRunner_Init(uint8_t *step, uint8_t *prev_step)
{
    *step = 0;
    *prev_step = 0;
    Matrix_ToggleLED(0, 0); // show first step LED
}

void StepRunner_Update(uint8_t *step, uint8_t *prev_step, uint8_t max_steps)
{
    Matrix_ToggleLED(*prev_step, 0); // OFF old
    if (max_steps == 0)
        max_steps = 32;
    *step = (*step + 1) % max_steps; // next step
    Matrix_ToggleLED(*step, 0);      // ON new
    *prev_step = *step;
}

// speed mapping from BPM
uint16_t GetStepDelayMs(uint8_t current_bpm)
{
    return 60000U / current_bpm;
}

// === int main ===============================================================

#define CURSOR_MIN_Y 1
#define CURSOR_MAX_Y 7
#define CURSOR_MIN_X 0
#define CURSOR_MAX_X 31

#define ROW_SHOW 0

int main()
{
    I2C_Init();
    LCD_I2C_Init();

    Rotary_Encoder_Init();
    Buzzer1_Init();
    Buzzer2_Init();
    Buzzer3_Init();

    Matrix_Init();
    Keypad_Init();

    LCD_I2C_SetCursor(0, 0);
    LCD_I2C_String("[PAUSE] BPM:120");
    LCD_I2C_SetCursor(1, 0);
    LCD_I2C_String("KEY: D(+2)");

    Matrix_Clear();
    Matrix_SetBrightness(10);

    // clear note buffer
    for (uint8_t y = 0; y < 8; y++)
        for (uint8_t x = 0; x < 32; x++)
            note_buf[y][x] = 0;

    sei();

    // === Step runner variables =============================================
    uint8_t step = 0;
    uint8_t prev_step = 0;
    StepRunner_Init(&step, &prev_step);

    // === Cursor variables ===================================================
    uint8_t cursor_x = 0;
    uint8_t cursor_y = 1;
    uint8_t cursor_visible = 0;

    uint16_t blink_timer_ms = 0;
    const uint16_t BLINK_PERIOD_MS = 200;

    // step timing
    uint16_t step_timer_ms = 0;

    char prev_key = 0;
    uint16_t hold_clear_timer = 0;

    while (1)
    {
        // ===== LCD update ==================================================
        if (lcd_update_flag)
        {
            lcd_update_flag = 0;
            LCD_Update_Status(cursor_y);
        }

        // ===== keypad input handling ======================================
        char key = Keypad_Read();

        // Edge detection for single presses
        if (key != 0 && key != prev_key)
        {
            // hide cursor before move
            if (cursor_visible)
            {
                Matrix_ToggleLED(cursor_x, cursor_y);
                cursor_visible = 0;
            }

            if (key == '4') // left
            {
                if (cursor_x > CURSOR_MIN_X)
                    cursor_x--;
                lcd_update_flag = 1;
            }
            else if (key == '6') // right
            {
                if (cursor_x < CURSOR_MAX_X)
                    cursor_x++;
                lcd_update_flag = 1;
            }
            else if (key == '2') // up
            {
                if (cursor_y > CURSOR_MIN_Y)
                    cursor_y--;
                lcd_update_flag = 1;
            }
            else if (key == '8') // down
            {
                if (cursor_y < CURSOR_MAX_Y)
                    cursor_y++;
                lcd_update_flag = 1;
            }
            else if (key == '5') // toggle note at cursor position
            {
                Note_Toggle(cursor_x, cursor_y);
            }
            else if (key == '1') // reset playhead to step 0
            {
                // Turn off current playhead LED
                if (matrix_buf[step / 8][0] & (1 << (7 - (step % 8))))
                {
                    Matrix_ToggleLED(step, 0);
                }

                step = 0;
                prev_step = 0;
                step_timer_ms = 0;

                // Turn on new playhead LED if not already on
                if (!(matrix_buf[0][0] & (1 << 7)))
                {
                    Matrix_ToggleLED(0, 0);
                }
            }
            else if (key == '7') // transpose up
            {
                if (key_transpose < 12)
                {
                    key_transpose++;
                    lcd_update_flag = 1;
                }
            }
            else if (key == '9') // transpose down
            {
                if (key_transpose > -12)
                {
                    key_transpose--;
                    lcd_update_flag = 1;
                }
            }

            blink_timer_ms = 0;
        }

        // Handle Button 3 Hold to Clear
        if (key == '3')
        {
            hold_clear_timer += 10;
            if (hold_clear_timer >= 2000) // 2 seconds
            {
                Note_ClearAll();

                // Reset cursor state so it doesn't "ghost" back on
                cursor_visible = 0;
                blink_timer_ms = 0;

                hold_clear_timer = 0;
            }
        }
        else
        {
            hold_clear_timer = 0;
        }

        prev_key = key;

        // ===== cursor blink ===============================================
        if (state == STATE_PAUSE)
        {
            blink_timer_ms += 10;
            if (blink_timer_ms >= BLINK_PERIOD_MS)
            {
                blink_timer_ms = 0;
                Matrix_ToggleLED(cursor_x, cursor_y);
                cursor_visible ^= 1;
            }
        }
        else
        {
            // If playing, hide cursor to avoid distraction
            if (cursor_visible)
            {
                Matrix_ToggleLED(cursor_x, cursor_y);
                cursor_visible = 0;
            }
            blink_timer_ms = 0;
        }

        // ===== step runner update (speed by BPM) ==========================
        if (state == STATE_PLAY)
        {
            uint16_t step_delay_ms = GetStepDelayMs(bpm);

            step_timer_ms += 10;
            if (step_timer_ms >= step_delay_ms)
            {
                step_timer_ms = 0;

                // limit loop to last active column + 1
                uint8_t last_note_col = Sequencer_GetLastActiveColumn();
                uint8_t loop_limit = (last_note_col < 3) ? 4 : (last_note_col + 1);

                StepRunner_Update(&step, &prev_step, loop_limit);

                // play note(s) in this column
                Sequencer_PlayColumn(step);
            }
        }
        else
        {
            step_timer_ms = 0;
            All_Stop();
        }

        _delay_ms(10);
    }
}