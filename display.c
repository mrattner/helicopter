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
 * Display the status of the PWM generator.
 * @param pulseWidth Current PWM pulse width
 * @param period Current PWM period
 */
void displayStatus (unsigned long pulseWidth, unsigned long period) {
	char string[30];
	int duty = (100 * pulseWidth + period / 2) / period;

	sprintf(string, "Duty cycle: %3d%%", duty);
	RIT128x96x4StringDraw(string, 4, 54, 15);
}
