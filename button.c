/**
 * button.c
 *
 * Support for buttons on the Stellaris LM3S1968 EVK
 */

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "stdlib.h"
#include "driverlib/gpio.h"

#include "button.h"
#include "buttonSet.h"

// Pin values for Port B (virtual buttons)
const unsigned char BUTTON_PINS_B[] = {
	GPIO_PIN_5, // virtual Up
	GPIO_PIN_6, // virtual Down
	GPIO_PIN_3, // virtual Left
	GPIO_PIN_2, // virtual Right
	GPIO_PIN_4, // virtual Select
	GPIO_PIN_1 // virtual Reset
};

// Pin values for Port G (physical buttons)
const unsigned char BUTTON_PINS_G[] = {
	GPIO_PIN_3, // physical Up
	GPIO_PIN_4, // physical Down
	GPIO_PIN_5, // physical Left
	GPIO_PIN_6, // physical Right
	GPIO_PIN_7, // physical Select
	0 // No GPIO pin for physical Reset
};

// Array of button structs
static button_t buttonsArray[6];

// Flag for whether initButSet has been called
static unsigned int initialised = 0;

// Number of SysTick interrupts in 1 msec.
static unsigned long sysTickmHz;

/**
 * Initialise the button instance for the specific button & pin.
 * Enable the port and pin for the button for polling only. Initialise state
 * of button to 'out'.
 * The desired GPIO port must be enabled with a call to SysCtlPeripheralEnable
 * before calling this function.
 */
void initButton (button_t *button, unsigned long ulPort, unsigned char ucPin) {
	button->ulPort = ulPort;
	button->ucPin = ucPin;
	button->iState = BUT_OUT;
}

/**
 * Initialise the button instances for the specific
 * buttons specified. Should only be called once the
 * SysCtlClock frequency is set.
 * @param buttons Bit pattern formed by UP_B, DOWN_B, etc.
 *  ORed together
 * @param port Either VIRTUAL_PORT or PHYSICAL_PORT
 * @param tickRateHz Rate of SysTick interrupts (set externally)
 */
void initButSet (unsigned char buttons, unsigned long port, unsigned int tickRateHz) {
	unsigned char ucPins = 0;
	int i;
	const unsigned char* pinArray;

	// Choose the pin array based on the port
	if (port == VIRTUAL_PORT) {
		pinArray = BUTTON_PINS_B;
	} else if (port == PHYSICAL_PORT){
		pinArray = BUTTON_PINS_G;
	}

	// Initialise the buttons to be inactive, with count 0
	for (i = 0; i < NUM_BUTTONS; i++) {
		buttonsArray[i].iState = BUT_INACTIVE;
		buttonsArray[i].uiCnt = 0;
	}

	// Store the number of SysTick interrupts in one msec:
	// (a thousandth of the number of SysTick interrupts per second)
	sysTickmHz = tickRateHz / 1000;

	// Determine the buttons and pins to activate
	if (buttons & UP_B) {
		initButton(&buttonsArray[UP], port, pinArray[UP]);
		ucPins |= pinArray[UP];
	}
	if (buttons & DOWN_B) {
		initButton(&buttonsArray[DOWN], port, pinArray[DOWN]);
		ucPins |= pinArray[DOWN];
	}
	if (buttons & LEFT_B) {
		initButton(&buttonsArray[LEFT], port, pinArray[LEFT]);
		ucPins |= pinArray[LEFT];
	}
	if (buttons & RIGHT_B) {
		initButton(&buttonsArray[RIGHT], port, pinArray[RIGHT]);
		ucPins |= pinArray[RIGHT];
	}
	if (buttons & SELECT_B) {
		initButton(&buttonsArray[SELECT], port, pinArray[SELECT]);
		ucPins |= pinArray[SELECT];
	}
	if ((buttons & RESET_B) && port == VIRTUAL_PORT) {
		initButton(&buttonsArray[RESET], port, pinArray[RESET]);
		ucPins |= pinArray[RESET];
	}

	// Configure the port and pin used. The peripheral in question
	// must be enabled beforehand.
	GPIODirModeSet(port, ucPins, GPIO_DIR_MODE_IN);
	// Use weak pull-up
	GPIOPadConfigSet(port, ucPins, GPIO_STRENGTH_2MA,
	   GPIO_PIN_TYPE_STD_WPU);

	initialised = 1;
}


/**
 * Designed to be called from the SysTick interrupt handler.
 * It has no return type or argument list. A call to initButSet()
 * is required before the first call to this.
 */
void updateButtons (void) {
	if (!initialised) { return; }

	unsigned char ucPins = 0;
	unsigned long pinRead, ulPort;
	int i;

	// Figure out which port and pins to read
	for (i = 0; i < NUM_BUTTONS; i++) {
		if (buttonsArray[i].iState != BUT_INACTIVE) {
			ulPort = buttonsArray[i].ulPort;
			ucPins |= buttonsArray[i].ucPin;
		}
	}

	pinRead = GPIOPinRead(ulPort, ucPins);

	for (i = 0; i < NUM_BUTTONS; i++) {
		// Isolate the read value of this button's pin.
		// If the value is 0, the button is pressed
		unsigned long pinVal = pinRead & buttonsArray[i].ucPin;

		switch (buttonsArray[i].iState) {
		case BUT_IN:
			if (pinVal && ulPort == PHYSICAL_PORT) {
				// Button is not being pressed (physical)
				if (buttonsArray[i].uiCnt >= sysTickmHz * BUT_HOLDOFF) {
					buttonsArray[i].iState = BUT_OUT;
					buttonsArray[i].uiCnt = 0;
				} else {
					buttonsArray[i].uiCnt += 1;
				}
			} else if (pinVal) {
				// Button is not being pressed (virtual)
				buttonsArray[i].iState = BUT_OUT;
			}
			break;
		case BUT_OUT:
			if (!pinVal && ulPort == PHYSICAL_PORT) {
				// Button is being pressed (physical)
				if (buttonsArray[i].uiCnt >= sysTickmHz * BUT_DETECT) {
					buttonsArray[i].iState = BUT_PUSHED;
					buttonsArray[i].uiCnt = 0;
				} else {
					buttonsArray[i].uiCnt += 1;
				}
			} else if (!pinVal) {
				// Button is being pressed (virtual)
				buttonsArray[i].iState = BUT_PUSHED;
			}
			break;
		default:
			// Button is inactive: do nothing
			break;
		}
	}
}


/**
 * Checks the specified individual button and returns true
 * (1) if the button is active and has state BUT_PUSHED (newly pushed)
 * and modifies state to BUT_IN. Returns false (0) otherwise.
 * @param button One of the enumerated buttons (e.g. UP)
 * @return true (1) if the button was newly pushed, false (0) otherwise
 */
unsigned int checkBut (unsigned int button) {
	if (buttonsArray[button].iState == BUT_PUSHED) {
		buttonsArray[button].iState = BUT_IN;
		return 1;
	} else {
		return 0;
	}
}


/**
 * Checks the current set of active buttons and returns true (>0)
 * if any of them have state = BUT_PUSHED. Value returned is an ORed
 * set of the bits representing the button(s) newly pushed,
 * e.g., if "UP" and "SELECT" have been recently pushed, the
 * value returned is (UP_B | SELECT_B). Otherwise returns false (0).
 * State of any pushed button is altered to BUT_IN.
 * @return a set of bits representing the button(s) that were pushed
 */
unsigned char anyButPushed (void) {
	unsigned char pushedButs = 0;

	if (buttonsArray[UP].iState == BUT_PUSHED) {
		buttonsArray[UP].iState = BUT_IN;
		pushedButs |= UP_B;
	}
	if (buttonsArray[DOWN].iState == BUT_PUSHED) {
		buttonsArray[DOWN].iState = BUT_IN;
		pushedButs |= DOWN_B;
	}

	return pushedButs;
}


/**
 * Alters the state of the specified button to BUT_OUT,
 * if it was previously BUT_INACTIVE, otherwise makes no change.
 * @param button One of the enumerated buttons (e.g. UP)
 */
void enableBut (unsigned int button) {
	if (buttonsArray[button].iState == BUT_INACTIVE) {
		buttonsArray[button].iState = BUT_OUT;
	}
}


/**
 * Alters the state of the specified button to BUT_INACTIVE.
 * @param button One of the enumerated buttons (e.g. UP)
 */
void disableBut (unsigned int button) {
	buttonsArray[button].iState = BUT_INACTIVE;
}
