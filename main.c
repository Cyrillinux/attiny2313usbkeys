/**
 * Project: AVR ATtiny USB Tutorial at http://codeandlife.com/
 * Author: Joonas Pihlajamaa, joonas.pihlajamaa@iki.fi
 * Base on V-USB example code by Christian Starkjohann
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v3 (see License.txt)
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#include "usbdrv.h"
#include "usbkeycodes.h"
#include "gamekeys.h"

// ************************
// *** USB HID ROUTINES ***
// ************************

// From Frank Zhao's USB Business Card project
// http://www.frank-zhao.com/cache/usbbusinesscard_details.php
PROGMEM char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x06, // USAGE (Keyboard)
    0xa1, 0x01, // COLLECTION (Application)
    0x75, 0x01, //   REPORT_SIZE (1)
    0x95, 0x08, //   REPORT_COUNT (8)
    0x05, 0x07, //   USAGE_PAGE (Keyboard)(Key Codes)
    0x19, 0xe0, //   USAGE_MINIMUM (Keyboard LeftControl)(224)
    0x29, 0xe7, //   USAGE_MAXIMUM (Keyboard Right GUI)(231)
    0x15, 0x00, //   LOGICAL_MINIMUM (0)
    0x25, 0x01, //   LOGICAL_MAXIMUM (1)
    0x81, 0x02, //   INPUT (Data,Var,Abs) ; Modifier byte
    0x95, 0x01, //   REPORT_COUNT (1)
    0x75, 0x08, //   REPORT_SIZE (8)
    0x81, 0x03, //   INPUT (Cnst,Var,Abs) ; Reserved byte
    0x95, 0x05, //   REPORT_COUNT (5)
    0x75, 0x01, //   REPORT_SIZE (1)
    0x05, 0x08, //   USAGE_PAGE (LEDs)
    0x19, 0x01, //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05, //   USAGE_MAXIMUM (Kana)
    0x91, 0x02, //   OUTPUT (Data,Var,Abs) ; LED report
    0x95, 0x01, //   REPORT_COUNT (1)
    0x75, 0x03, //   REPORT_SIZE (3)
    0x91, 0x03, //   OUTPUT (Cnst,Var,Abs) ; LED report padding
    0x95, 0x06, //   REPORT_COUNT (6)
    0x75, 0x08, //   REPORT_SIZE (8)
    0x15, 0x00, //   LOGICAL_MINIMUM (0)
    0x25, 0x65, //   LOGICAL_MAXIMUM (101)
    0x05, 0x07, //   USAGE_PAGE (Keyboard)(Key Codes)
    0x19, 0x00, //   USAGE_MINIMUM (Reserved (no event indicated))(0)
    0x29, 0x65, //   USAGE_MAXIMUM (Keyboard Application)(101)
    0x81, 0x00, //   INPUT (Data,Ary,Abs)
    0xc0 // END_COLLECTION
};

typedef struct {
    uint8_t modifier;
    uint8_t reserved;
    uint8_t keycode[6];
} keyboard_report_t;

static keyboard_report_t keyboardReport; // sent to PC
static uchar idleRate; // repeat rate for keyboards

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
    usbRequest_t *rq = (void *) data;

    if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
        switch (rq->bRequest) {
            case USBRQ_HID_GET_REPORT: // send "no keys pressed" if asked here
                // wValue: ReportType (highbyte), ReportID (lowbyte)
                usbMsgPtr = (void *) &keyboardReport; // we only have this one
                keyboardReport.modifier = 0;
                keyboardReport.keycode[0] = 0;
                return sizeof (keyboardReport);
            case USBRQ_HID_SET_REPORT: // if wLength == 1, should be LED state
                return (rq->wLength.word == 1) ? USB_NO_MSG : 0;
            case USBRQ_HID_GET_IDLE: // send idle rate to PC as required by spec
                usbMsgPtr = &idleRate;
                return 1;
            case USBRQ_HID_SET_IDLE: // save idle rate as required by spec
                idleRate = rq->wValue.bytes[1];
                return 0;
        }
    }

    return 0; // by default don't return any data
}

usbMsgLen_t usbFunctionWrite(uint8_t * data, uchar len) {
    return 1; // Data read, not expecting more
}

#define NUM_LOCK 1
#define CAPS_LOCK 2
#define SCROLL_LOCK 4

// the SN74HCT165N is fast enough so we don't need a delay, this saves a few bytes
//#define PULSE_WIDTH_USEC	5

uchar readShiftRegisters() {
    uchar i;
	uchar offset = 0;
	uchar retval = 0;
	
    // Trigger a parallel load to latch the state of the data lines
    PORT_SHIFTREG &= ~(1 << PIN_SHLD);
	// the SN74HCT165N is fast enough so we don't need a delay, this saves a few bytes
    //_delay_us(PULSE_WIDTH_USEC);
    PORT_SHIFTREG |= (1 << PIN_SHLD);

    // Loop to read each bit value from the serial out line of the shift register
	// we forge the keyboard report directly, to safe some memory
    for (i = 0; i < NUMBER_OF_BUTTONS; i++) {
		
		if (PORT_IN_SHIFTREG & (1 << PIN_QH)) {
			// button is pressed
			if (keyboardReport.keycode[offset] != buttonCodes[i]) {
				// button wasn't pressed before or another button was pressed at this offset
				keyboardReport.keycode[offset] = buttonCodes[i];
				offset++; 	
				retval = 1;
			} else {
				// same button is still pressed, don't trigger an USB update but increase the offset
				offset++;
			}
		} else {
			// button is not pressed
			if (keyboardReport.keycode[offset] == buttonCodes[i]) {
				// previously there was THIS button pressed at this offset, unset
				keyboardReport.keycode[offset] = KEY_NONE;
				// no need to increase the offset, this key-slot can be re-used
				retval = 1;
			}
		}

        // Pulse the clock (rising edge shifts the next bit).
        PORT_SHIFTREG |= (1 << PIN_CLK);
		// the SN74HCT165N is fast enough so we don't need a delay, this saves a few bytes
        //_delay_us(PULSE_WIDTH_USEC);
        PORT_SHIFTREG &= ~(1 << PIN_CLK);
		
		if (offset >= 6) {
			break;
		}
    }
	
	/*
		we've gone through all shift registers. make sure all leftover key-slots are reset
		
		this is necessary for key sequences like this: none -> A -> A && B -> B -> none
		the resulting keycode sequences would be:
			00 00 00 00 00 00 00 00			none
			00 00 04 00 00 00 00 00			A
			00 00 04 05 00 00 00 00			A && B
			00 00 05 05 00 00 00 00 		B 		note the stale B on 2nd position.
			00 00 00 05 00 00 00 00			none	stale B is still there
			
			so this loop cleans that up
	*/
	for (i = offset; i < 6; i++) {
		keyboardReport.keycode[i] = KEY_NONE;
	}
	
	return retval;	
}

int main() {
	uchar updateNeeded = 0;

    // configure output of 74HTC165N shift register as input
    REGISTER_SHIFTREG &= ~(1 << PIN_QH);
    // clock and shift/load pins as output
    REGISTER_SHIFTREG |= (1 << PIN_CLK);
    REGISTER_SHIFTREG |= (1 << PIN_SHLD);

    // set shift/load high to prevent reading of data
    PORT_SHIFTREG |= (1 << PIN_SHLD);
    // set clock pin low
    PORT_SHIFTREG &= ~(1 << PIN_CLK);

    // clear report initially - unwrapped the for loop to save a few bytes
	keyboardReport.modifier = 0;
	keyboardReport.reserved = 0;
	keyboardReport.keycode[0] = KEY_NONE;
	keyboardReport.keycode[1] = KEY_NONE;
	keyboardReport.keycode[2] = KEY_NONE;
	keyboardReport.keycode[3] = KEY_NONE;
	keyboardReport.keycode[4] = KEY_NONE;
	keyboardReport.keycode[5] = KEY_NONE;
	
    wdt_enable(WDTO_1S); // enable 1s watchdog timer
    
	usbInit();
	
    usbDeviceDisconnect(); // enforce re-enumeration
	// wait 500 ms, watchdog is set to 1 sec, so no wdt_reset() calls are required
	_delay_ms(500);	
    usbDeviceConnect();

    sei(); // Enable interrupts after re-enumeration

    while (1) {
        wdt_reset(); // keep the watchdog happy
        usbPoll();
		
		/*
			read all shift registers and fill the keyboardReport accordingly.
			
			we cannot get rid of the updateNeeded variable:
			it might be that an update becomes needed but the USB interrupt isn't ready yet.
			in that case we need to wait until it does. if we'd directly use the call to readShiftRegisters()
			in the if-statement below we'd not trigger the change at all because at the time
			the USB interrupt is ready, the changed button configuration will already have
			been written to keyboardReport and the function call would return 0 again since
			there is no new change.
			*/
        updateNeeded = readShiftRegisters();
	
        if (updateNeeded && usbInterruptIsReady()) {
            usbSetInterrupt((void *) &keyboardReport, sizeof(keyboardReport));
			updateNeeded = 0;
        }
    }

    return 0;
}
