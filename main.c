/*
 * LatchingRelaisV1.c
 *
 * Created: 20.04.2017 20:41:00
 * Author : Fritzsche
 * ATtiny85-20 @ 8,000 MHz
 *
 * https://www.mikrocontroller.net/articles/Multitasking
 * https://www.mikrocontroller.net/articles/Statemachine
 * https://www.mikrocontroller.net/articles/Bitmanipulation
 * https://www.mikrocontroller.net/articles/AVR-GCC-Tutorial
 * 
 * LED + 1KOhm  @ PB3 and PB4 to VCC
 * Taster nach GND an PB0 with pullup
*/

#include <avr/io.h>
#include "avr/interrupt.h"
#include "util/delay.h"
#include <stdbool.h>
#include "bitio.h"

#define F_CPU 1000000	//delivery state

unsigned char  state  = 1;
int16_t        delcnt = 0;
int8_t		   ptt    = 0;
volatile uint8_t flag_1ms;

uint8_t ptt_read(void) {
	if (!(PINB & (1<<PINB0)))
		return 1;                //when grounded
	else
		return 0;
}

void stateMachine()
{
	switch( state ) {
		case 1:        //State 1 is initialization state, PB1 must be High for one sec, for initialization of the Relay, PB3 must be low for LED
			if (delcnt <999) {
				delcnt++;
				PORTB |= (1<<PB0) | (1<<PB1) | (1<<PB4); // 0b010011 LED1 (PB3=0) ON, Coil1 on
				state = 1;
			} else {
				delcnt = 0;
				PORTB &= ~(1<<PB1);                      // 0b010001 Coil1 off
				PORTB |=  (1<<PB3);                      // 0b011001 LED1 (PB3=1) off
				state = 2;
			}
			break;
		case 2:	     //State 2 is NC, LED is blinking for 0,5sec
			if (delcnt < 499) {
				delcnt++;
				state = 2;
			} else if (delcnt < 999) {
				delcnt++;
				state = 2;
				PORTB &= ~(1<<PB3);						//0b010001 LED1 (PB3=0) on
			} else {
				delcnt = 0;
				state = 2;
				PORTB |= (1<<PB3);						//0b011001 LED1 (PB3=1) off
			}
			if (ptt) {
				delcnt =0;
				state = 3;
				PORTB |= (1<<PB3);						//0b011001 LED1 (PB3=1) off
			}
			break;
		case 3:     // switch from NC to NO, LED2 is on
			if (delcnt <999) {
				delcnt++;
				PORTB  = (1<<PB0) | (1<<PB2) | (1<<PB3); // 0b001101 LED2 ON, Coil2 on
				if (ptt) {
					state = 3;							// nothing else
					} else {
					delcnt = 0;
					state = 5;							// LED2 off, Coil2 off
					PORTB = (1<<PB0) | (1<<PB3) | (1<<PB4); 
					}
			} else {
				delcnt = 0;
				PORTB &= ~(1<<PB2);                      // !!!0b010001 Coil2 off
				PORTB |=  (1<<PB4);                      // !!!0b011001 LED2 off
				if (ptt) {
					state = 4;							 //nothing else to do
				} else {
					state = 5;							 //nothing else to do
				}
			}
			break;
		case 4: // stationary state NO closed, LED 2 is blinking
			if (delcnt < 499) {
				delcnt++;
				if (ptt) {
					state = 4;
				} else {
					delcnt=0;
					state = 5;
				} 
			} else if (delcnt < 999) {
				delcnt++;
				if (ptt) {
					state = 4;
					PORTB &= ~(1<<PB4);						//0b001001 LED2 (PB4=0) on	
				} else {
					delcnt=0;
					state = 5;
					PORTB = (1<<PB0) | (1<<PB3) | (1<<PB4); //0b011001 LED2 (PB4=1) off
				}
			} else {
				delcnt = 0;
				PORTB |= (1<<PB4);						    //0b011001 LED1 (PB4=1) off
				if (ptt) {
					state = 4;					
				} else {
					state = 5;
				}
			}
			break;
		case 5: // switch from 4 NO closed to 2 NC closed, LED1 is on
			if (delcnt <999) {
				delcnt++;
				PORTB = (1<<PB0) | (1<<PB1) | (1<<PB4);     //0b010011 LED1 is on
				if (ptt) {
					delcnt=0;
					state = 3;
					PORTB = (1<<PB0) | (1<<PB3) | (1<<PB4); //0b011001
				} else {
					state = 5;
					//
				}
			} else {
				delcnt = 0;
				PORTB = (1<<PB0) | (1<<PB3) | (1<<PB4);  //0b011001 LEDs off, Coils off
				if (ptt) {
					state = 3;
				} else {
					state = 2;
				}
			}
			break;
	}
}

int main(void) {
	

	// IO initialization
	// Define pull-ups and set outputs high
	// PORTB PB0 is Input and needs pull up
	PORTB |=  (1<<PB0);
	DDRB  |=  (1 << DDB1) | (1 << DDB2) | (1 << DDB3) | (1 << DDB4); 
	
	// Timer 0 initialization, CTC, prescaler 8 (was 64)
	// CTC Mode (WGM01)
	TCCR0A = (1<<WGM01);
	TCCR0B = (1<<CS01);		// | (1<<CS00);
	OCR0A  = 125;			// 1ms
	TIMSK |= (1<<OCIE0A);

	// Interrupts globally released

	sei();
	// Main Loop for ever

	while (1) {
		 if (flag_1ms) {
			flag_1ms=0;
			ptt = ptt_read();
			stateMachine();
			//_delay_ms(1);       // 1 ms delay
		 }
	}
}

// Interruptserviceroutine für Timer 0
// hier 1ms

ISR(TIMER0_COMPA_vect) {
	if (flag_1ms) {
		// Laufzeit der Tasks >2ms, Fehlersignalisierung
		// PB1 auf HIGH, Programm stoppen
		// PORTB |= (1<<PB1);
		while(1);
	}
	flag_1ms = 1;
}