/*
 * Brushless to Brushed
 *
 * Created: 14-7-2018 10:16:29
 * Author : Gin-Lung Cheng
 */ 


#include "hkf80a.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <math.h>
#include <avr/eeprom.h>


#define FORWARD_LOW LOW_A_PIN
#define FORWARD_LOW_PORT LOW_A_PORT

#define FORWARD_HIGH HIGH_C_PIN
#define FORWARD_HIGH_PORT HIGH_C_PORT

#define BACKWARD_LOW LOW_C_PIN
#define BACKWARD_LOW_PORT LOW_C_PORT

#define BACKWARD_HIGH HIGH_A_PIN
#define BACKWARD_HIGH_PORT HIGH_A_PORT

//Define RC States
#define UNINITIALIZED 0
#define FORWARD 1
#define BACKWARD 2
#define BRAKE 3

void setupPwmOutput();
void setupRcInput();

void goForwards();
void goBackwards();
void brake();

void motorBeep(uint8_t length);

inline void enableForwardHigh();
inline void disableForwardHigh();
inline void enableForwardLow();
inline void disableForwardLow();
inline void enableBackwardHigh();
inline void disableBackwardHigh();
inline void enableBackwardLow();
inline void disableBackwardLow();


volatile uint8_t state = UNINITIALIZED;
volatile uint8_t time0 = 0;
volatile uint16_t startHighTime = 0;
volatile uint8_t timeout = 50;
volatile uint8_t targetState = UNINITIALIZED;
volatile uint8_t targetPwm = 0;


/*

 To make avrdude work: zadig install driver: libusbk
 avrdude -c usbasp -p m8
 avrdude -c usbasp -p m8 -U flash:w:BrushlessToBrushed.hex

 bs.inc = F80A
 
 F20A - only n-channels
 F80A - p and n-channels
 
 p-channel (HIGH) on = connects to voltage
 n-channel (LOW) on = connects to ground
 
 p-channel on == negative voltage gate, source=voltage, drain=ground
 n-channel on == positive voltage gate, source=ground, drain=voltage
 
 Forward = C High, A Low
 Reverse = C Low,  A High

*/

int main(void) {
    setupMotorOutput();
    setupPwmOutput();
    setupRcInput();

    sei(); // Enable interrupts.

    motorBeep(1);

    while (1) {
        if (state != targetState) {
            if (targetState == FORWARD && state == BRAKE) {
                OCR2 = 0;
                goForwards();
            } else if (targetState == BACKWARD && state == BRAKE) {
                OCR2 = 0;
                goBackwards();
            } else {
                brake();
            }
        } else {
            OCR2 = targetPwm;
        }
    }
}


void setupMotorOutput() {
    // Set A pins as outputs.
    SET_HIGH_A_PORT |= (1 << HIGH_A);
    SET_LOW_A_PORT |= (1 << LOW_A);

    // Set C pins as outputs.
    SET_HIGH_C_PORT |= (1 << HIGH_C);
    SET_LOW_C_PORT |= (1 << LOW_C);
}


void setupPwmOutput() {
    // Setup PWM Timer2.
    OCR2 = 0x00;

    // Set DDB3 as Timer2 PWM output.
    DDRB |= (1 << DDB3);

    // Timer2 output compare interrupt.
    TIMSK |= (1 << OCIE2);

    // Timer2 overflow interrupt.
    TIMSK |= (1 << TOIE2);

    // Prescaler 8 = 2MHz PWM cycle.
    TCCR2 = (1 << CS21);
}


void setupRcInput() {
    // Setup RC PWM Timer0.

    // Prescaler 64 = 250KHz (8bit counter = 1ms).
    TCCR0 |= (1 << CS01 | 1 << CS00);

    // Timer0 overflow interrupt.
    TIMSK |= (1 << TOIE0);

    // Set PD2 as RC input pin.
    DDRD &= ~(1 << DDD2);

    // Interrupt on changes (any edge) of RC input pin.
    MCUCR |= (1 << ISC00);

    // Turn on RC input pin interrupt.
    GICR |= (1 << INT0);
}


ISR(TIMER2_OVF_vect) {
    // Entering ON part of PWM.
    if (OCR2 > 0) {
        if (state == FORWARD) {
            enableForwardHigh();
        } else if (state == BACKWARD) {
            enableBackwardHigh();
        }
    }
}


ISR(TIMER2_COMP_vect) {
    // Entering OFF part of PWM.
    if (OCR2 < 255) {
        if (state != BRAKE) {
            disableForwardHigh();
            disableBackwardHigh();
        }
    }
}


ISR(TIMER0_OVF_vect) {
    // Triggers every 1 ms.
    if (timeout == 0) {
        if (state != BRAKE) {
            targetState = BRAKE;
        }
    } else {
        timeout--;
    }

    time0++;
}


ISR(INT0_vect) {
    // Triggered when RC input received.

    uint16_t time = TCNT0;
    time = time + time0 * 256;

    if (RC_PORT & (1 << RC_PIN)) {
        // Edge ON.
        startHighTime = time;
        timeout = 50;
    } else {
        // Edge OFF.
        uint16_t elapsedHighTime = time - startHighTime;
        if (elapsedHighTime > RC_LOW && elapsedHighTime < RC_HIGH) {
            if (elapsedHighTime > RC_MID + RC_DEADZONE) {
                targetPwm = (elapsedHighTime - RC_MID) * 256 / (RC_HIGH - RC_MID) ;
                if (targetPwm > 255) {
                    targetPwm = 255;
                }
                targetState = FORWARD;
            } else if (elapsedHighTime < RC_MID - RC_DEADZONE) {
                targetPwm = (RC_MID - elapsedHighTime) * 256 / (RC_MID - RC_LOW) ;
                if (targetPwm < 0) {
                    targetPwm = 0;
                }
                targetState = BACKWARD;
            } else {
                targetPwm = 0;
                targetState = BRAKE;
            }
        }
    }
}


void goForwards() {
    if (state != FORWARD) {
        cli();
        disableBackwardHigh();
        disableBackwardLow();
        _delay_us(200);
        enableForwardLow();
        _delay_us(200);
        enableForwardHigh();
        state = FORWARD;
        sei();
    }
}


void goBackwards() {
    if (state != BACKWARD) {
        cli();
        disableForwardHigh();
        disableForwardLow();
        _delay_us(200);
        enableBackwardLow();
        _delay_us(200);
        enableBackwardHigh();
        state = BACKWARD;
        sei();
    }
}


void brake() {
    if (state != BRAKE) {
        cli();
        disableForwardHigh();
        disableBackwardHigh();
        _delay_us(200);
        enableForwardLow();
        enableBackwardLow();
        state = BRAKE;
        _delay_ms(10);
        sei();
    }
}


void motorBeep(uint8_t length) {
    OCR2 = 10;

    for (uint8_t i = 0; i < length; i++) {
        if (i % 2) {
            goForwards();
        } else {
            goBackwards();
        }
        _delay_ms(200);
        brake();
        _delay_ms(300);
    }

    OCR2 = 0;
}


inline void enableForwardHigh() {
    FORWARD_HIGH_PORT |= (1 << FORWARD_HIGH);
}


inline void disableForwardHigh() {
    FORWARD_HIGH_PORT &= ~(1 << FORWARD_HIGH);
}


inline void enableForwardLow() {
    FORWARD_LOW_PORT |= (1 << FORWARD_LOW);
}


inline void disableForwardLow() {
    FORWARD_LOW_PORT &= ~(1 << FORWARD_LOW);
}


inline void enableBackwardHigh() {
    BACKWARD_HIGH_PORT |= (1 << BACKWARD_HIGH);
}


inline void disableBackwardHigh() {
    BACKWARD_HIGH_PORT &= ~(1 << BACKWARD_HIGH);
}


inline void enableBackwardLow() {
    BACKWARD_LOW_PORT |= (1 << BACKWARD_LOW);
}


inline void disableBackwardLow() {
    BACKWARD_LOW_PORT &= ~(1 << BACKWARD_LOW);
}