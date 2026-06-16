//#define F_CPU 8000000L
//
//#include <avr/io.h>
//#include <util/delay.h>
//
//int main(void) {
//    
//    // LED as Output
//    DDRB |= (1 << PB1);
//    
//    // Buttons as Input
//    DDRD &= ~(1 << PD2);
//    DDRD &= ~(1 << PD3);
//    
//    // Enable internal pull-up resistors
//    PORTD |= (1 << PD2);
//    PORTD |= (1 << PD3);
//    
//    uint8_t LED_State = 0;
//    
//    while (1) {
//        // Check Button Pressed
//        if ( !(PIND & (1 << PD2)) || !(PIND & (1 << PD3)) ) {
//            _delay_ms(200); // Debounce
//            
//            // Wait until button released
//            while( !(PIND & (1 << PD2)) || !(PIND & (1 << PD3)) );
//            
//            // Toggle LED State
//            LED_State ^= 1;
//            
//            if(LED_State){
//                PORTB |= (1 << PB1);
//            } else {
//                PORTB &= ~(1 << PB1);
//            }
//
//        }       
//    }
//}
