/*
 * motorControl.c
 *
 * Control module for the main and tail motors of the helicopter.
 *
 * Author: J. Shaw and M. Rattner
 */

#include "globals.h"
#include "motorControl.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#include "driverlib/sysctl.h"
#include "driverlib/pwm.h"
#include "driverlib/gpio.h"

/**
 * Initialise the PWM generators. Should be called after the associated
 * GPIO pins have been enabled for output.
 * PWM generator 0: Controls PWM1 (Main rotor)
 * PWM generator 2: Controls PWM4 (Tail rotor)
 */
void initPWMchan (void) {
	if (initialised) {
		return;
	}
	unsigned long period;

	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM);

	// Configure both PWM generators' counting mode and synchronisation mode
	PWMGenConfigure(PWM_BASE, PWM_GEN_0,
					PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);
	PWMGenConfigure(PWM_BASE, PWM_GEN_2,
				PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);

	// Compute the PWM period based on the system clock.
	SysCtlPWMClockSet(SYSCTL_PWMDIV_4);

	// We set the PWM clock to be 1/4 the system clock, so set the period
	// to be 1/4 the system clock divided by the desired frequency.
	period = SysCtlClockGet() / 4 / PWM_RATE_HZ;
	// Generator 0
	PWMGenPeriodSet(PWM_BASE, PWM_GEN_0, period);
	PWMPulseWidthSet(PWM_BASE, PWM_OUT_1, period * MAIN_INITIAL_DUTY100 / 10000);
	// Generator 2
	PWMGenPeriodSet(PWM_BASE, PWM_GEN_2, period);
	PWMPulseWidthSet(PWM_BASE, PWM_OUT_4, period * TAIL_INITIAL_DUTY100 / 10000);

	// Enable the PWM output signals.
	PWMOutputState(PWM_BASE, PWM_OUT_1_BIT | PWM_OUT_4_BIT, false);

	// Enable the PWM generators.
	PWMGenEnable(PWM_BASE, PWM_GEN_0);
	PWMGenEnable(PWM_BASE, PWM_GEN_2);

	initialised = 1;
}

/**
 * If the helicopter is at a low enough altitude, turn off the motors.
 */
void powerDown (void) {
	_desiredAltitude = 0;

	if (_avgAltitude < 2) {
		PWMOutputState(PWM_BASE, PWM_OUT_1_BIT | PWM_OUT_4_BIT, false);
		_heliState = HELI_OFF;
	}
}

/**
 * Turns on the motors and sets duty cycle of both to the initial amount.
 */
void powerUp (void) {
	if (!initialised) {
		return;
	}
	PWMOutputState(PWM_BASE, PWM_OUT_1_BIT | PWM_OUT_4_BIT, true);
	setDutyCycle100(MAIN_ROTOR, MAIN_INITIAL_DUTY100);
	setDutyCycle100(TAIL_ROTOR, TAIL_INITIAL_DUTY100);

	_heliState = HELI_ON;
}

/**
 * Sets the PWM duty cycle to be the duty cycle %. Has built in
 * safety at 5% or 95% if dutyCycle set above 95% or below 5%.
 * @param rotor Either MAIN_ROTOR or TAIL_ROTOR
 * @param dutyCycle100 The duty cycle % times 100
 */
void setDutyCycle100 (unsigned long rotor, unsigned int dutyCycle100) {
	if (!initialised) {
		return;
	}
	unsigned long period = (rotor == MAIN_ROTOR) ?
				PWMGenPeriodGet(PWM_BASE, PWM_GEN_0) // main rotor PWM
				: PWMGenPeriodGet(PWM_BASE, PWM_GEN_2); // tail rotor PWM

	if (dutyCycle100 < 500) {
		dutyCycle100 = 500;
	} else if (dutyCycle100 > 9500) {
		dutyCycle100 = 9500;
	}
	unsigned long newPulseWidth = (dutyCycle100 * period + 5000) / 10000;

	// Set the pulse width according to the desired duty cycle
	PWMPulseWidthSet(PWM_BASE, rotor, newPulseWidth);
}

/**
 * Get the current duty cycle of the specified rotor PWM.
 * @param rotor Either MAIN_ROTOR or TAIL_ROTOR
 * @return The current duty cycle % of that rotor's PWM channel, * 100
 */
unsigned int getDutyCycle100 (unsigned long rotor) {
	unsigned long currentPulseWidth = PWMPulseWidthGet(PWM_BASE, rotor);
	unsigned long currentPeriod = (rotor == MAIN_ROTOR) ?
			PWMGenPeriodGet(PWM_BASE, PWM_GEN_0) // main rotor PWM
			: PWMGenPeriodGet(PWM_BASE, PWM_GEN_2); // tail rotor PWM

	return (10000 * currentPulseWidth + currentPeriod / 2) / currentPeriod;
}

/**
 * Adjusts the PWM duty cycle of the main rotor to control the altitude.
 */
void altitudeControl (void) {
	if (!initialised) {
		return;
	}
	unsigned int currentDutyCycle100 = getDutyCycle100(MAIN_ROTOR);
	if (_avgAltitude < _desiredAltitude) {
		setDutyCycle100(MAIN_ROTOR, currentDutyCycle100 + 70);
	} else if (_avgAltitude > _desiredAltitude) {
		setDutyCycle100(MAIN_ROTOR, currentDutyCycle100 - 70);
	}
}

/**
 * Adjusts the PWM duty cycle of the tail rotor to control the yaw.
 */
void yawControl (void) {
	if (!initialised) {
		return;
	}
	int currentDutyCycle100 = getDutyCycle100(TAIL_ROTOR);

	setDutyCycle100(TAIL_ROTOR, (_desiredYaw100 - _yaw100) * 1);
}
