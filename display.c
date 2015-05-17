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
	char string[30];

	if (_avgAltitude <= 100 && _avgAltitude >= 0) {
		sprintf(string, "Altitude = %3d%%", _avgAltitude);
	} else {
		sprintf(string, "Invalid altitude");
	}
	RIT128x96x4StringDraw(string, 4, 14, 15);
}

/**
 * Display the yaw of the heli rig in degrees, relative to start
 * position.
 */
void displayYaw () {
	char string[30];
	int degrees = (_yaw + 50) / 100;
	sprintf(string, "Yaw = %3d deg", degrees);
	RIT128x96x4StringDraw(string, 4, 34, 15);
}

/**
 * Display the duty cycle of the PWM generators.
 * @param mainDuty Duty cycle of the main rotor
 * @param tailDuty Duty cycle of the tail rotor
 */
void displayPWMStatus (unsigned int mainDuty, unsigned int tailDuty) {
	char string[30];
	// TODO: Display tail rotor duty cycle as well
	sprintf(string, "Main rotor: %3d%%", mainDuty);
	RIT128x96x4StringDraw(string, 4, 54, 15);
}
