// defines the vars for the shift registers
#define REGISTER_SHIFTREG	DDRB
#define PORT_SHIFTREG		PORTB
#define PORT_IN_SHIFTREG	PINB
#define PIN_CLK				PB2
#define PIN_SHLD			PB3
#define PIN_QH				PB4

#define NUMBER_OF_BUTTONS 8

/* 
	this defines the button assignment for each bit in the shift registers.
	the bit read first from the registers must be at index 0 of the array.
	
	for a list of (almost) all USB keycodes see usbkeycodes.h
*/
uchar buttonCodes[NUMBER_OF_BUTTONS] = {KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H};
