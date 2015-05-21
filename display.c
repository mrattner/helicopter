/******************************************************************************
*
* display.c - Display control module for the helicopter program.
*
* Author: J. Shaw and M. Rattner
**/

#include "globals.h"
#include "inc/hw_types.h"
#include "driverlib/pwm.h"
#include "drivers/rit128x96x4.h"
#include "stdio.h"

/**
 * Initialise the OLED display with an SSI clock frequency of 1 MHz.
 */
void initDisplay (void) {
	RIT128x96x4Init(1000000);
}

/**
 * Display the altitude of the heli rig. The measured value from
 * the ADC will be ~1-2 V. Decreasing voltage = increasing altitude.
 */
void displayAltitude () {
	char string[20];

	if (_avgAltitude <= 100 && _avgAltitude >= 0) {
		snprintf(string, 20, "Altitude: %3d%%  ", _avgAltitude);
	} else {
		snprintf(string, 20, "Invalid altitude");
	}
	RIT128x96x4StringDraw(string, 4, 14, 15);
}

/**
 * Display the yaw of the heli rig in degrees, relative to start
 * position.
 */
void displayYaw () {
	char string[20];
	int degrees = (_yaw + 50) / 100;
	snprintf(string, 20, "Yaw: %4d deg", degrees);
	RIT128x96x4StringDraw(string, 4, 34, 15);
}

/**
 * Display the duty cycle of the PWM generators.
 * @param mainDuty Duty cycle of the main rotor
 * @param tailDuty Duty cycle of the tail rotor
 */
void displayPWMStatus (unsigned int mainDuty, unsigned int tailDuty) {
	char mainString[20];
	char tailString[20];
	if (mainDuty == 1000 || tailDuty == 1000) {
		snprintf(mainString, 20, "PWM uninitialised");
		RIT128x96x4StringDraw(mainString, 4, 54, 15);
	}
	else {
		snprintf(mainString, 20, "Main rotor: %3d%%", mainDuty);
		snprintf(tailString, 20, "Tail rotor: %3d%%", tailDuty);
		RIT128x96x4StringDraw(mainString, 4, 54, 15);
		RIT128x96x4StringDraw(tailString, 4, 64, 15);
	}
}
