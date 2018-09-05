// CPU frequency
#define F_CPU 16000000UL // 16MHz

// Channel A
#define SET_HIGH_A_PORT DDRD
#define HIGH_A_PORT PORTD
#define HIGH_A_PIN 4 // pin PD4 - pFet

#define SET_LOW_A_PORT DDRD
#define LOW_A_PORT PORTD
#define LOW_A_PIN 5 // pin PD5 - nFet

// Channel B (not used)
#define SET_HIGH_B_PORT DDRC
#define HIGH_B_PORT PORTC
#define HIGH_B_PIN 5 // pin PC5 - pFet

#define SET_LOW_B_PORT DDRC
#define LOW_B_PORT PORTC
#define LOW_B_PIN 4 // pin PC4 - nFet

// Channel C
#define SET_HIGH_C_PORT DDRC
#define HIGH_C_PORT PORTC
#define HIGH_C_PIN 3 // pin PC3 - pFet

#define SET_LOW_C_PORT DDRB
#define LOW_C_PORT PORTB
#define LOW_C_PIN 0 // pin PB0 - nFet


// RC input
#define RC_PORT PIND
#define RC_PIN 2

#define RC_LOW 275
#define RC_MID 375
#define RC_HIGH 475
#define RC_DEADZONE 10