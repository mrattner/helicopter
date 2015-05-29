/******************************************************************************
*
* buttonCheck.c - Button control logic.
*
* Author: J. Shaw and M. Rattner
**/

#include "buttonCheck.h"
#include "buttonSet.h"
#include "button.h"
#include "globals.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#include "driverlib/sysctl.h"

/**
 * Initialise the buttons.
 * @param port Either PHYSICAL or VIRTUAL
 */
void initButtons (int port) {
	if (port == VIRTUAL) {
		// Enable GPIO port B for the virtual buttons
		SysCtlPeripheralReset(SYSCTL_PERIPH_GPIOB);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
		// The button set being used is UP, DOWN, LEFT, RIGHT,
		// SELECT, and RESET on the virtual port
		initButSet(UP_B | DOWN_B | LEFT_B | RIGHT_B | SELECT_B | RESET_B,
				VIRTUAL_PORT, SYSTICK_RATE_HZ);
	} else if (port == PHYSICAL) {
		// Enable GPIO port G for the physical buttons
		SysCtlPeripheralReset(SYSCTL_PERIPH_GPIOG);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
		// The button set being used is UP, DOWN, LEFT, RIGHT,
		// and SELECT on the physical port
		initButSet(UP_B | DOWN_B | LEFT_B | RIGHT_B | SELECT_B,
				PHYSICAL_PORT, SYSTICK_RATE_HZ);
	}
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
	if (checkBut(UP) && _heliState == HELI_ON) {
		_desiredAltitude += ALTITUDE_STEP;
		if (_desiredAltitude > 100) {
			_desiredAltitude = 100;
		}
	}
	else if (checkBut(DOWN) && _heliState == HELI_ON) {
		_desiredAltitude -= ALTITUDE_STEP;
		if (_desiredAltitude < 0) {
			_desiredAltitude = 0;
		}
	}
	else if (checkBut(LEFT) && _heliState == HELI_ON) {
		_desiredYaw100 -= YAW_STEP_100;
		if (_desiredYaw100 < -34500) {
			_desiredYaw100 = -34500;
		}
	}
	else if (checkBut(RIGHT) && _heliState == HELI_ON) {
		_desiredYaw100 += YAW_STEP_100;
		if (_desiredYaw100 > 34500) {
			_desiredYaw100 = 34500;
		}
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
