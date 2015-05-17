/******************************************************************************
*
* buttonCheck.c - Button control logic.
*
* Author: J. Shaw and M. Rattner
**/

#include "buttonSet.h"
#include "button.h"
#include "globals.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#include "driverlib/sysctl.h"

/**
 * Initialise the buttons.
 */
void initButtons (void) {
	// Enable GPIO port B for the virtual buttons
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	// The button set being used is UP, DOWN, LEFT, RIGHT,
	// SELECT, and RESET on the virtual port
	initButSet(UP_B | DOWN_B | LEFT_B | RIGHT_B | SELECT_B | RESET_B,
			VIRTUAL_PORT, SYSTICK_RATE_HZ);
}

/**
 * Checks each button for a press and performs an associated action:
 *  UP: Increases desired altitude
 *  DOWN: Decreases desired altitude
 *  LEFT: Increments yaw counterclockwise
 *  RIGHT: Increments yaw clockwise
 *  SELECT: Starts up or lands the helicopter
 *  RESET: Perform a "soft" system reset via SysCtl
 */
void checkButtons (void) {
	if (checkBut(UP)) {
		_desiredAltitude = _avgAltitude + ALTITUDE_STEP;
	}
	else if (checkBut(DOWN)) {
		_desiredAltitude = _avgAltitude - ALTITUDE_STEP;
	}
	else if (checkBut(LEFT)) {
		_desiredYaw = _yaw - YAW_STEP;
	}
	else if (checkBut(RIGHT)) {
		_desiredYaw = _yaw + YAW_STEP;
	}
	else if (checkBut(SELECT)) {
		switch (_heliState) {
		case HELI_OFF:
			_heliState = HELI_STARTING;
			break;
		case HELI_ON:
			_heliState = HELI_STOPPING;
			break;
		default:
			// If helicopter is currently starting or stopping, do nothing
			return;
		}
	}
	else if (checkBut(RESET)) {
		SysCtlReset();
	}
}
