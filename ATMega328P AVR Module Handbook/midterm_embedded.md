# CPE-324 Embedded Systems — Key Snippets Summary

> ATmega328P (AVR)

---

## Base Code

### 1. Header มาตรฐาน

```c
#include <avr/io.h>        // Register definitions (DDRB, PORTB, etc.)
#include <avr/interrupt.h> // ISR(), sei(), cli()
// #include <util/delay.h>    // _delay_ms(), _delay_us() ❌ [Banned]
#include <stdio.h>         // sprintf(), snprintf()
```

---

### 2. Bit Manipulation - init pins

```c
DDRB  |= (1 << PB0);    // PB0 เป็น OUTPUT
DDRB  &= ~(1 << PB0);   // PB0 เป็น INPUT

PORTB |= (1 << PB0);    // PB0 = HIGH / เปิด Internal Pull-up
PORTB &= ~(1 << PB0);   // PB0 = LOW

if (PINB & (1 << PB0))  // อ่านค่า PB0
```

`|=` → เปิด bit, `&= ~` → ปิด bit, ไม่กระทบ bit อื่น

---

### 3. LCD 4-bit Mode (ใช้ใน Assign 5, 6, 7, 8, 9)

#### ตัวอย่างการประกาศ Pin
```c
#define LCD_E_PORT    PORTB
#define LCD_E_PIN     PB2
#define LCD_RS_PORT   PORTB
#define LCD_RS_PIN    PB1
#define LCD_DATA_PORT PORTC
```

#### ส่ง Enable Pulse ให้ LCD รับข้อมูล
```c
void commitData() {
    LCD_E_PORT |= (1 << LCD_E_PIN);   // E = 1 (rising edge)
    _delay_us(1);
    LCD_E_PORT &= ~(1 << LCD_E_PIN);  // E = 0 (falling edge = latch data)
    _delay_us(100);
}
```
> LCD รับข้อมูลที่ขอบขาลง (falling edge) ของ Enable pin

#### ส่ง Command ด้วย 4-bit mode (แยก 2 ครั้ง)

**กรณี 1: LCD D4-D7 → MCU PORT bit 0-3**
```c
void SendLCDCommand(uint8_t command) {
    LCD_RS_PORT &= ~(1 << LCD_RS_PIN);                          // RS = 0 → Command mode
    LCD_DATA_PORT = (LCD_DATA_PORT & 0xF0) | (command >> 4);    // High nibble → bit 0-3
    commitData();
    LCD_DATA_PORT = (LCD_DATA_PORT & 0xF0) | (command & 0x0F);  // Low nibble  → bit 0-3
    commitData();
    _delay_ms(2);
}

void SendLCDData(uint8_t data) {
    LCD_RS_PORT |= (1 << LCD_RS_PIN);                           // RS = 1 → Data mode
    LCD_DATA_PORT = (LCD_DATA_PORT & 0xF0) | (data >> 4);
    commitData();
    LCD_DATA_PORT = (LCD_DATA_PORT & 0xF0) | (data & 0x0F);
    commitData();
    _delay_ms(2);
}
```

**กรณี 2: LCD D4-D7 → MCU PORT bit 4-7**
```c
void SendLCDCommand(uint8_t command) {
    LCD_RS_PORT &= ~(1 << LCD_RS_PIN);                          // RS = 0 → Command mode
    LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (command & 0xF0);  // High nibble → bit 4-7
    commitData();
    LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (command << 4);    // Low nibble  → bit 4-7
    commitData();
    _delay_ms(2);
}

void SendLCDData(uint8_t data) {
    LCD_RS_PORT |= (1 << LCD_RS_PIN);                           // RS = 1 → Data mode
    LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (data & 0xF0);
    commitData();
    LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (data << 4);
    commitData();
    _delay_ms(2);
}
```
> 4-bit mode = ใช้แค่ 4 เส้น data (LCD pin D4-D7) แทน 8 เส้น ประหยัด pin MCU  
> ต้องส่ง 2 รอบ (nibble สูง → nibble ต่ำ) ต่อ 1 byte  
> wiring ต่างกัน → mask/shift ต่างกัน: bit 0-3 ใช้ mask `0xF0`, bit 4-7 ใช้ mask `0x0F`

#### Sequence เริ่มต้น LCD
```c
void initLCD() {
    _delay_ms(20);            // รอ LCD power-up
    SendLCDCommand(0x03);     // Wake-up sequence (3 ครั้ง)
    _delay_ms(5);
    SendLCDCommand(0x03);
    _delay_us(200);
    SendLCDCommand(0x03);
    _delay_us(200);
    SendLCDCommand(0x02);     // เข้า 4-bit mode
    SendLCDCommand(0x28);     // 4-bit, 2 lines, 5x8 font
    SendLCDCommand(0x0E);     // Display ON, Cursor ON
    SendLCDCommand(0x01);     // Clear
    SendLCDCommand(0x80);     // Cursor home (row 1)
}
```
> `0x02` = switch เข้า 4-bit  
> `0xC0` = set cursor ขึ้นบรรทัด 2  
> `0x80` = บรรทัด 1

---

### 4. ตัวแปร `volatile` — ใช้กับทุก ISR

```c
volatile uint8_t State = 0;
volatile uint16_t adcValue = 0;
volatile uint8_t adcReady = 0;
```

> **ทำไมต้อง volatile?**  
> ตัวแปรที่เปลี่ยนค่าใน ISR อาจถูก compiler optimize ออก (cache ใน register)  
> `volatile` บอก compiler: "อย่า optimize — ค่านี้อาจเปลี่ยนได้ตลอดเวลา"

---

### 5. Global Interrupt Enable

```c
sei();   // Set Enable Interrupt → เปิด interrupt ทั้งหมด
cli();   // Clear Interrupt → ปิด interrupt ชั่วคราว (ใน critical section)
```

---

## Assignment 5 — Keypad, LCD, General Purpose Input/Output

### Key Snippet: สแกน Keypad Matrix 4×4

Keypad 8 เส้น ต่อที่ PORTD ทั้งหมด: Row (4 เส้น) → PD0-3, Column (4 เส้น) → PD4-7

```c
char keyMap[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

char scanKeypad() {
    for (uint8_t row = 0; row < 4; row++) {
        // ดึง row ทั้งหมดเป็น HIGH แล้วดึง row ที่ต้องการลง LOW
        DDRD  |= 0x0F;          // PD0-3 = Output (rows)
        PORTD |= 0x0F;          // ทุก row = HIGH
        PORTD &= ~(1 << row);   // row ปัจจุบัน = LOW

        _delay_us(5);

        for (uint8_t col = 0; col < 4; col++) {
            if (!(PIND & (1 << (col + 4)))) {  // ถ้า column อ่านได้ LOW = มีการกด
                _delay_ms(200);                 // Debounce
                return keyMap[row][col];
            }
        }
    }
    return '\0';  // ไม่มีปุ่มกด
}
```

Init ก่อนใช้งาน:
```c
void initKey() {
    DDRD  |= 0x0F;   // PD0-3 = Output (rows)
    DDRD  &= 0x0F;   // PD4-7 = Input  (columns)
    PORTD |= 0xF0;   // เปิด Pull-up บน columns
}
```

> **หลักการ:** ดึง Row ลง LOW ทีละแถว → ถ้ามีปุ่มกด Column ที่เชื่อมจะอ่านได้ LOW  
> เหมือน grid circuit: กด = ต่อวงจร row-col ที่นั้น

---

## Assignment 6 — External Interrupt & ADC Interrupt

### Key Snippet: INT0 — ขัดจังหวะจาก Button

```c
void initINT0() {
    EICRA |= (1 << ISC01);   // ISC01=1, ISC00=0 → Falling edge
    EIMSK |= (1 << INT0);    // เปิด INT0
    sei();
}

ISR(INT0_vect) {
    State ^= 1;        // Toggle state (XOR flip bit)
    _delay_ms(50);     // Debounce ใน ISR
}
```

> **ทำไม Falling Edge?** ปุ่มต่อ GND + Pull-up → กด = LOW, ปล่อย = HIGH  
> Falling edge = จับ "ช่วงแรกที่กด" ไม่ใช่ตอนปล่อย

### Key Snippet: ADC Interrupt

```c
void initADC() {
    ADMUX |= (1 << REFS0);           // Reference = AVcc (5V)
    ADCSRA |= (1 << ADEN)            // เปิด ADC
           |  (1 << ADIE)            // เปิด ADC Interrupt
           |  (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);  // Prescaler /128
}

ISR(ADC_vect) {
    adcValue = ADC;   // อ่านค่า 10-bit (0–1023) จาก register ADC
    adcReady = 1;     // ส่งสัญญาณว่าค่าพร้อมแล้ว
}
```

> ADC ต้องการ clock **50–200kHz** เพื่อความละเอียด 10-bit เต็ม — prescaler ขึ้นกับ `F_CPU`:
>
> | F_CPU | Prescaler | ADC clock | bits ที่ใช้ |
> |-------|-----------|-----------|------------|
> | 1MHz  | /8  (ADPS1\|ADPS0)              | 125kHz ✓ | แนะนำ |
> | 8MHz  | /64 (ADPS2\|ADPS1)              | 125kHz ✓ | แนะนำ |
> | 8MHz  | /128 (ADPS2\|ADPS1\|ADPS0)     | 62.5kHz ✓ | ใช้ได้ |
> | 16MHz | /128 (ADPS2\|ADPS1\|ADPS0)     | 125kHz ✓ | แนะนำ |

```c
// ใน main loop
uint32_t percent = ((uint32_t)adcValue * 100) / 1023;
```

> ADC อ่านแรงดัน → แปลงเป็น 10-bit digital (0=0V, 1023=5V)  
> ต้องใช้ `uint32_t` ก่อน คูณ เพื่อป้องกัน overflow

---

## Assignment 7 — Timer Interrupt (Overflow)

### Key Snippet: Timer1 Overflow ทุก 2 วินาที

```c
#define F_CPU 1000000UL

// Timer1 = 16-bit → 2^16 = 65536 states (นับ 0..65535 แล้ว overflow)
// prescaler /1024: ไม่มีแล้ว overflow ทุก 65ms เท่านั้น ไม่ถึง 2 วินาที
//                  ใส่แล้ว 1 tick = 1024/1MHz = 1.024ms → รองรับได้ถึง ~67 วินาที
// tick ที่ต้องการ  = 2s / 1.024ms = 1953 tick
// TIMER1_INIT     = 65536 - 1953 = 63583

#define TIMER1_INIT 63583

void initTimer1() {
    TCCR1A = 0;                            // Normal mode
    TCCR1B = (1 << CS12) | (1 << CS10);  // Prescaler 1024
    TCNT1  = TIMER1_INIT;                 // โหลดค่าเริ่มต้น
    TIMSK1 = (1 << TOIE1);               // เปิด Overflow interrupt
}

ISR(TIMER1_OVF_vect) {
    TCNT1 = TIMER1_INIT;  // Reload ทุกครั้งที่ overflow
    // ... ทำงาน
}
```

> **หลักการ:** Timer นับขึ้น → overflow ที่ 65536 → ISR ทำงาน  
> Reload = เลื่อนจุดเริ่มต้นให้ overflow เร็ว/ช้าขึ้น

### Key Snippet: Short/Long Press Detection (Running Strip)

```c
volatile uint16_t Press_Time = 0;
volatile uint8_t  Button_Pressed = 0;

ISR(TIMER1_OVF_vect) {        // ทำงานทุก 100ms
    TCNT1 = TIMER1_INIT;

    if (!(PIND & (1 << PD2))) {   // ถ้ากดปุ่มอยู่
        Press_Time++;
        Button_Pressed = 1;
    } else if (Button_Pressed) {  // เพิ่งปล่อยปุ่ม
        if (Press_Time < 30)      // < 30 tick = 3 วินาที → Short press
            Running ^= 1;
        else                       // >= 30 tick → Long press
            Off_Mode ^= 1;
        Press_Time    = 0;
        Button_Pressed = 0;
    }
}
```

> นับ tick ขณะกด → ตัดสินใจตอนปล่อย  
> Short press / Long press = pattern ที่ใช้บ่อยมากใน UX ของ embedded

---

## Assignment 8 — PWM & Input Capture

### Key Snippet: PWM ด้วย Timer0 (Fast PWM Mode)

```c
void InitPWM() {
    TCCR0A = (1 << WGM00) | (1 << WGM01);  // Fast PWM (mode 3)
    TCCR0A |= (1 << COM0A1);               // Clear OC0A on compare match
    TCCR0B = (1 << CS01);                  // Prescaler /8
    OCR0A = 0;                             // Duty 0%
    DDRD  |= (1 << PD6);                   // OC0A ผูกกับ PD6 โดย hardware — pin อื่นไม่ได้
}

// ใน ISR(ADC_vect):
ISR(ADC_vect) {
    uint8_t Duty = ADC >> 2;  // 10-bit (0-1023) → 8-bit (0-255)
    OCR0A = Duty;             // ปรับ duty cycle ตาม ADC
    ADCSRA |= (1 << ADSC);    // เริ่ม conversion ถัดไป
}
```

> Fast PWM: นับ 0→255 ซ้ำ → pin HIGH ตอนนับถึง OCR0A, LOW ตอน overflow  
> `ADC >> 2` = หาร 4 (เลื่อนขวา 2 bit) เพื่อ scale ลง 8-bit

### Key Snippet: Input Capture — วัดความกว้างของ Pulse (Ultrasonic HC-SR04)

```c
volatile uint16_t StartTick = 0, EndTick = 0;
volatile uint8_t  MeasurementReady = 0;

void initInputCapture() {
    TCCR1A = 0;               // Normal Couter Mode
    TCCR1B = (1 << CS11);     // Prescaler /8 → 1 tick = 8µs (ที่ 1MHz) หรือ 1µs (ที่ 8MHz)
    TCCR1B |= (1 << ICES1);   // เริ่มด้วย Rising edge
    TIMSK1 |= (1 << ICIE1);   // เปิด Input Capture interrupt
}

ISR(TIMER1_CAPT_vect) {
    if (TCCR1B & (1 << ICES1)) {       // Rising edge = เริ่มต้น pulse
        StartTick = ICR1;
        TCCR1B &= ~(1 << ICES1);       // เปลี่ยนเป็นจับ Falling edge
    } else {                            // Falling edge = จบ pulse
        EndTick = ICR1;
        MeasurementReady = 1;
        TCCR1B |= (1 << ICES1);        // กลับไปจับ Rising edge
    }
}

// คำนวณระยะทาง (cm):
// Distance = (EndTick - StartTick) × tick_time × 34000 / 2
uint32_t Duration = EndTick - StartTick;
uint32_t Distance = (Duration * 34000UL) / 250000UL;  // ที่ prescaler /8, 1MHz
```

> **หลักการ HC-SR04:** ส่ง trigger 10µs → เซนเซอร์ยิงคลื่นและส่งพัลส์ (Echo) กลับมา  
> ความกว้าง pulse = เวลาที่เสียงเดินทางไป-กลับ → หารด้วยความเร็วเสียง = ระยะทาง

---

## Assignment 9 — I2C & SPI Communication

### Key Snippet: I2C Master (อ่าน RTC DS1307)

```c
void I2C_Init() {
    TWBR = 32;   // SCL = 8MHz / (16 + 2×32×1) = ~100kHz
}

void I2C_Start() {
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);  // ส่ง START condition
    while (!(TWCR & (1<<TWINT)));             // รอ complete
}

void I2C_Stop() {
    TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);  // ส่ง STOP condition
}

void I2C_Write(uint8_t data) {
    TWDR = data;
    TWCR = (1<<TWINT)|(1<<TWEN);             // ส่ง byte
    while (!(TWCR & (1<<TWINT)));            // รอ ACK
}

uint8_t I2C_Read_ACK()  { /* อ่านแล้วส่ง ACK  (ยังมี byte ต่อมา)  */ }
uint8_t I2C_Read_NACK() { /* อ่านแล้วส่ง NACK (byte สุดท้าย)      */ }

// Sequence อ่านเวลาจาก DS1307:
void ReadRTC() {
    I2C_Start();
    I2C_Write((0x68 << 1) | 0);  // Address 0x68 + Write
    I2C_Write(0x00);              // Register pointer: เริ่มที่ 0 (seconds)
    I2C_Stop();

    I2C_Start();
    I2C_Write((0x68 << 1) | 1);  // Address 0x68 + Read
    sec   = BCD_to_DEC(I2C_Read_ACK() & 0x7F);
    min   = BCD_to_DEC(I2C_Read_ACK());
    hour  = BCD_to_DEC(I2C_Read_ACK() & 0x3F);
    // ...
    year  = BCD_to_DEC(I2C_Read_NACK());  // byte สุดท้าย → NACK
    I2C_Stop();
}

uint8_t BCD_to_DEC(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}
```

> **I2C Protocol:** START → Address+RW → DATA bytes → STOP  
> DS1307 เก็บเวลาเป็น BCD (Binary-Coded Decimal): `0x59` = 59 นาที  
> ACK = "ส่งต่อมาได้", NACK = "พอแล้ว/ไม่รับ"

### Key Snippet: SPI Master + Interrupt (อ่าน ADC จาก MCP3201)

```c
void SPI_Init() {
    DDRB |= (1<<PB5)|(1<<PB3)|(1<<PB2);  // SCK, MOSI, CS = Output
    DDRB &= ~(1<<PB4);                   // MISO = Input
    PORTB |= (1<<PB2);                   // CS = HIGH (inactive)

    SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0)|(1<<SPIE);  // Enable, Master, /16, Interrupt
    SPSR = (1<<SPI2X);  // ×2 speed → prescaler /8
}

volatile uint8_t  spiHighByte, spiLowByte;
volatile uint8_t  spiByteCount = 0;
volatile uint16_t adcValue = 0;

ISR(SPI_STC_vect) {   // ทำงานทุกครั้งที่ส่ง/รับ 1 byte เสร็จ
    uint8_t received = SPDR;

    if (spiByteCount == 0) {
        spiHighByte = received;
        spiByteCount++;
        SPDR = 0;              // เริ่ม byte ที่ 2
    } else {
        spiLowByte = received;
        PORTB |= (1<<PB2);     // CS = HIGH → จบ transaction

        // MCP3201: 12-bit result กระจายอยู่ใน 2 bytes (ต้องเลื่อน bit)
        adcValue = (((uint16_t)(spiHighByte & 0x1F)) << 8 | spiLowByte) >> 1;
        adcReady = 1;
        spiByteCount = 0;
    }
}

// เริ่ม SPI transaction:
PORTB &= ~(1<<PB2);  // CS = LOW → เลือก slave
SPDR = 0;            // ส่ง dummy byte เพื่อ clock out ข้อมูล
```

> **SPI:** ส่ง-รับพร้อมกัน (full duplex) ผ่าน SCK clock  
> MCP3201 ส่งค่า 12-bit กลับใน 2 byte: bit กระจาย ต้อง mask + shift จัดตำแหน่ง  
> CS (Chip Select) LOW = เลือก device นั้น, HIGH = ปล่อย

---

## สูตร Sensor → LCD

### ADC Raw → หน่วยต่างๆ

```c
// แรงดัน (mV) — Vref = 5V, 10-bit ADC
uint32_t voltage_mV = (uint32_t)adcValue * 5000 / 1023;

// อุณหภูมิ (°C) — LM35: 10mV ต่อ 1°C
uint32_t temp_C = voltage_mV / 10;

// ความสว่าง / ค่าทั่วไป (%)
uint32_t percent = (uint32_t)adcValue * 100 / 1023;

// ระยะทาง HC-SR04 (cm) — Timer prescaler /8, F_CPU = 8MHz → 1 tick = 1µs
// ความเร็วเสียง = 34000 cm/s, ÷2 เพราะเดินทางไป-กลับ
uint32_t distance_cm = (uint32_t)duration * 34000 / 2000000;
```

> ใช้ `uint32_t` ก่อนคูณทุกครั้ง ป้องกัน overflow ระหว่างคำนวณ

### แสดงบน LCD ด้วย sprintf

```c
#include <stdio.h>

char buf[16];
sprintf(buf, "Temp: %lu C", temp_C);

for (uint8_t i = 0; buf[i] != '\0'; i++)
    SendLCDData(buf[i]);
```

> `%lu` = unsigned long (uint32_t), `%d` = int, `%u` = unsigned int

---

## สรุปภาพรวม

| Assignment | หัวข้อ | Peripheral หลัก | Pattern สำคัญ |
|---|---|---|---|
| **5** | GPIO + I/O | Keypad, LCD | Matrix scan, 4-bit LCD |
| **6** | Interrupts | INT0, ADC | ISR, volatile, falling edge |
| **7** | Timer Overflow | Timer1 | Reload value, press duration |
| **8** | PWM + Capture | Timer0, Timer1 ICP | Fast PWM, pulse width measure |
| **9** | Serial Comms | I2C (TWI), SPI | Protocol sequence, BCD, bit extraction |

### Flow ของ Interrupt-Driven Program

```
main() {
    init hardware
    sei()           ← เปิด interrupt
    while(1) {
        if(flag) {  ← flag ถูก set โดย ISR
            process data
            flag = 0
        }
    }
}

ISR(XXX_vect) {     ← ทำงานเมื่อมี event
    เก็บข้อมูล
    set flag = 1    ← บอก main loop
}
```

> **Best practice:** ISR ทำน้อยที่สุด → set flag → main loop ทำงานหนัก  
> หลีกเลี่ยง delay ใน ISR (block interrupt อื่น)

---

## Custom Delay via Timer1 (ไม่ใช้ built-in)

> F_CPU = 1,000,000 Hz → prescaler = 1 → **1 tick = 1 µs**  
> ใช้ **Timer1 (16-bit)** → รองรับ delay สูงสุด 65535 µs ต่อครั้ง

### หลักการ — CTC Mode + Poll Flag

```
TCCR1B: WGM12=1 (CTC), CS10=1 (no prescaler)
OCR1A : ค่า compare → timer นับ 0…OCR1A แล้วเซ็ต OCF1A
TCNT1 : reset เป็น 0 ก่อนนับทุกครั้ง
TIFR1 : อ่าน/เคลียร์ flag OCF1A ด้วย write 1
```

### Implementation

```c
// ---------- delay_us ----------
// Timer1 CTC, prescaler=1 → 1 tick = 1 µs @ 1MHz
void delay_us(uint16_t us) {
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS10);  // CTC, no prescaler
    OCR1A  = us - 1;                        // compare match เมื่อนับครบ us รอบ
    TCNT1  = 0;                             // reset counter
    TIFR1 |= (1 << OCF1A);                 // เคลียร์ flag ก่อน (write 1 to clear)
    while (!(TIFR1 & (1 << OCF1A)));       // รอจนกว่า compare match
    TCCR1B = 0;                             // หยุด timer
}

// ---------- delay_ms ----------
// 1 ms = 1000 µs = 1000 ticks → OCR1A = 999
void delay_ms(uint16_t ms) {
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS10);  // CTC, no prescaler
    OCR1A  = 999;                           // 1000 ticks = 1 ms
    TCNT1  = 0;
    while (ms--) {
        TIFR1 |= (1 << OCF1A);             // เคลียร์ flag ทุกรอบ ms
        while (!(TIFR1 & (1 << OCF1A)));   // รอ 1 ms
    }
    TCCR1B = 0;                             // หยุด timer
}
```

### ตัวอย่างใช้งาน

```c
delay_ms(500);      // รอ 500 ms
delay_us(250);      // รอ 250 µs
```

### สรุป Register

| Register | ค่าที่ set | ความหมาย |
|---|---|---|
| `TCCR1A` | `0` | ไม่ใช้ OC output |
| `TCCR1B` | `(1<<WGM12)\|(1<<CS10)` | CTC mode, prescaler = 1 |
| `OCR1A` | `us-1` / `999` | compare value |
| `TCNT1` | `0` | reset ก่อนนับ |
| `TIFR1` | `(1<<OCF1A)` | write 1 → clear flag |
