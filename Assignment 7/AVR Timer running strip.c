#define F_CPU 1000000UL
#include <avr/io.h>
#include <avr/interrupt.h>

#define LED1 PORTB1
#define LED2 PORTB2
#define LED3 PORTB3
#define SWITCH PORTD2

// Overflow Interrupt Every 100 ms
#define TIMER1_INIT 65438 // 65536 - 65438 ~ 98 ms 

// For Circuit Mode
volatile uint8_t Running = 1;
volatile uint8_t Off_Mode = 0;

// For Running LED
volatile uint8_t Step = 0; // Which LED Light up
volatile uint8_t Timer_Tick = 0; // How Long

// For Switching Mode
volatile uint8_t Button_Pressed = 0; // Boolean
volatile uint16_t Press_Time = 0; // How Long

void SetLED(uint8_t s)
{
    PORTB &= ~((1 << LED1) | (1 << LED2) | (1 << LED3));

    switch(s)
    {
        case 0: PORTB |= (1 << LED1); break;
        case 1: PORTB |= (1 << LED2); break;
        case 2: PORTB |= (1 << LED3); break;
        case 3: PORTB |= (1 << LED2); break;
    }
}

ISR(TIMER1_OVF_vect)
{
    TCNT1 = TIMER1_INIT;
    if(Running && !Off_Mode)
    {
        // Running Code
        Timer_Tick++;
        if(Timer_Tick >= 3)
        {
            Timer_Tick = 0;
            
            SetLED(Step);
            Step++;
            if(Step > 3)
                Step = 0;
        }
    }
    
    if(Button_Pressed)
    {
        if(!(PIND & (1 << SWITCH)))   // Still Pressed
        {
            Press_Time++;
        
            if(Press_Time >= 30)
            {
                Off_Mode ^= 1;

                if(Off_Mode)
                    PORTB &= ~((1<<LED1)|(1<<LED2)|(1<<LED3));
            
                Button_Pressed = 0;
                Press_Time = 0;
            }
        }
        
        else // Released
        {
            if(Press_Time < 30)
            {
                Running ^= 1;
            }
            Button_Pressed = 0;
        }
    }
}

ISR(INT0_vect)
{
    Button_Pressed = 1;
    Press_Time = 0;
}

void Timer1_Init()
{
    // Timer1: Normal Mode, Prescaler 1024
    TCCR1A = (0 << WGM10) | (0 << WGM11);
    TCCR1B = (1 << CS12) | (1 << CS10);
    TCNT1 = TIMER1_INIT;
}

void INT0_Init()
{   
    EICRA |= (1 << ISC01) | (0 << ISC00); // INT0 falling edge
    EIMSK |= (1 << INT0); // Enable INT0
    
    TIMSK1 = (1 << TOIE1);  // Open Overflow Interrupt
    sei(); // Global Interrupt
}
    
int main(void)
{
    DDRB |= (1 << LED1) | (1 << LED2) | (1 << LED3);
    
    DDRD &= ~(1 << SWITCH);
    PORTD |= (1 << SWITCH);

    Timer1_Init();
    INT0_Init();

    while(1)
    {
    }
}