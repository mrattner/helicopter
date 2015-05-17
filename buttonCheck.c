/******************************************************************************
*
* buttonCheck.c - Button control logic.
*
* Author: J. Shaw and M. Rattner
**/

#include "buttonSet.h"
#include "button.h"
#include "globals.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"


void checkButtons() {
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
