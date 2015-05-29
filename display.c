/******************************************************************************
*
* display.c - Display control module for the helicopter program.
*
* Author: J. Shaw and M. Rattner
**/

#include "globals.h"
#include "inc/hw_types.h"
#include "drivers/rit128x96x4.h"
#include "stdio.h"

/**
 * Initialise the OLED display with an SSI clock frequency of 200 kHz.
 * Note that this can only be called after serialLink's initConsole()
 * function because initConsole() resets GPIOA.
 */
void initDisplay (void) {
	RIT128x96x4Init(200000);
}

/**
 * Display the altitude of the heli rig. The measured value from
 * the ADC will be ~1-2 V. Decreasing voltage = increasing altitude.
 */
void displayAltitude () {
	char actualString[20];
	char desiredString[20];
	char stateString[20];

	snprintf(actualString, 20, "Altitude: %d%%        ", _avgAltitude);
	snprintf(desiredString, 20, "Desired: %d%%   ", _desiredAltitude);
	snprintf(stateString, 20, "Heli state: %d", _heliState);

	RIT128x96x4StringDraw(actualString, 4, 14, 15);
	RIT128x96x4StringDraw(desiredString, 4, 24, 15);
	RIT128x96x4StringDraw(stateString, 4, 74, 15);
}

/**
 * Display the yaw of the heli rig in degrees, relative to start
 * position.
 */
void displayYaw () {
	char actualString[20];
	char desiredString[20];
	snprintf(actualString, 20, "Yaw*100: %d    ", _yaw100);
	snprintf(desiredString, 20, "Desired*100: %d    ", _desiredYaw100);

	RIT128x96x4StringDraw(actualString, 4, 34, 15);
	RIT128x96x4StringDraw(desiredString, 4, 44, 15);
}

/**
 * Display the duty cycle of the PWM generators.
 * @param mainDuty Duty cycle of the main rotor
 * @param tailDuty Duty cycle of the tail rotor
 */
void displayPWMStatus (unsigned int mainDuty100, unsigned int tailDuty100) {
	char mainString[20];
	char tailString[20];
	snprintf(mainString, 20, "Main rotor: %d   ", mainDuty100);
	snprintf(tailString, 20, "Tail rotor: %d   ", tailDuty100);

	RIT128x96x4StringDraw(mainString, 4, 54, 15);
	RIT128x96x4StringDraw(tailString, 4, 64, 15);
}
