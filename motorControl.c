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

	SysCtlPeripheralReset(SYSCTL_PERIPH_PWM);
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

	// Disable the PWM output signals until we want the heli to take off.
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
	unsigned int mainDuty = getDutyCycle100(MAIN_ROTOR);

	// Check that the heli is close to the ground and that the main rotor
	// has a low duty cycle before shutting off.
	if (_avgAltitude < 5 && mainDuty <= MIN_DUTY100) {
		PWMOutputState(PWM_BASE, PWM_OUT_1_BIT | PWM_OUT_4_BIT, false);
		setDutyCycle100(MAIN_ROTOR, MAIN_INITIAL_DUTY100);
		setDutyCycle100(TAIL_ROTOR, TAIL_INITIAL_DUTY100);
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

	setDutyCycle100(MAIN_ROTOR, MAIN_INITIAL_DUTY100);
	setDutyCycle100(TAIL_ROTOR, TAIL_INITIAL_DUTY100);
	PWMOutputState(PWM_BASE, PWM_OUT_1_BIT | PWM_OUT_4_BIT, true);
	_heliState = HELI_ON;
}

/**
 * Sets the PWM duty cycle to be the duty cycle %. Has built in
 * safety at 5% or 95% if dutyCycle set above 95% or below 5%.
 * @param rotor Either MAIN_ROTOR or TAIL_ROTOR
 * @param dutyCycle100 The desired duty cycle % times 100
 */
void setDutyCycle100 (unsigned long rotor, unsigned int dutyCycle100) {
	if (!initialised) {
		return;
	}
	unsigned long period = (rotor == MAIN_ROTOR) ?
				PWMGenPeriodGet(PWM_BASE, PWM_GEN_0) // main rotor PWM
				: PWMGenPeriodGet(PWM_BASE, PWM_GEN_2); // tail rotor PWM

	// If the desired duty cycle is below 5% or above 95%, hold.
	if (dutyCycle100 < MIN_DUTY100) {
		dutyCycle100 = MIN_DUTY100;
	} else if (dutyCycle100 > MAX_DUTY100) {
		dutyCycle100 = MAX_DUTY100;
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
	unsigned long pulseWidth = PWMPulseWidthGet(PWM_BASE, rotor);
	unsigned long period = (rotor == MAIN_ROTOR) ?
			PWMGenPeriodGet(PWM_BASE, PWM_GEN_0) // main rotor PWM
			: PWMGenPeriodGet(PWM_BASE, PWM_GEN_2); // tail rotor PWM

	return (10000 * pulseWidth + period / 2) / period;
}

/**
 * Adjusts duty cycle from current duty cycle.
 * Used for testing the open loop response of the helicopter motors.
 * @param rotor Either MAIN_ROTOR or TAIL_ROTOR
 * @param amount Percentage * 100 to adjust the duty cycle
 */
void changeDutyCycle (unsigned long rotor, signed int amount) {
	unsigned long currentPulseWidth = PWMPulseWidthGet(PWM_BASE, rotor);
	unsigned long period = (rotor == MAIN_ROTOR) ?
				PWMGenPeriodGet(PWM_BASE, PWM_GEN_0) // main rotor PWM
				: PWMGenPeriodGet(PWM_BASE, PWM_GEN_2); // tail rotor PWM

	// Don't allow the duty cycle to change too much at once
	if (amount > MAX_DUTY_CHANGE100) {
		amount = MAX_DUTY_CHANGE100;
	} else if (amount < -MAX_DUTY_CHANGE100) {
		amount = -MAX_DUTY_CHANGE100;
	}

	signed long pulseWidthChange = (amount * (signed long)period + 5000) / 10000;
	signed long newPulseWidth = currentPulseWidth + pulseWidthChange;
	unsigned long minPulseWidth = (MIN_DUTY100 * period + 5000) / 10000;
	unsigned long maxPulseWidth = (MAX_DUTY100 * period + 5000) / 10000;

	// Don't allow the duty cycle to be <5% or >95%
	if (newPulseWidth < minPulseWidth) {
		newPulseWidth = minPulseWidth;
	} else if (newPulseWidth > maxPulseWidth) {
		newPulseWidth = maxPulseWidth;
	}

	PWMPulseWidthSet(PWM_BASE, rotor, newPulseWidth);
}

/**
 * Adjusts the PWM duty cycle of the main rotor to control the altitude.
 */
void altitudeControl (void) {
	if (!initialised) {
		return;
	}
	unsigned int mainDuty100 = getDutyCycle100(MAIN_ROTOR);
	static signed long errorIntegrated = 200;
	int error = _desiredAltitude - _avgAltitude;

	// Bypass normal altitude control if the heli is landing
	if (_heliState == HELI_STOPPING && mainDuty100 > MIN_DUTY100) {
		changeDutyCycle(MAIN_ROTOR, -MAX_DUTY_CHANGE100);
		errorIntegrated = 0;
		return;
	}

	// bias = 1500; Kp = 25; Ki = 25/2
	int newDuty100 = 1500 + error * 25 + errorIntegrated * 25 / 2;

	// Don't allow duty cycle to change too much at once
	if (newDuty100 > (signed int)mainDuty100 + MAX_DUTY_CHANGE100) {
		newDuty100 = mainDuty100 + MAX_DUTY_CHANGE100;
	} else if (newDuty100 < (signed int)mainDuty100 - MAX_DUTY_CHANGE100) {
		newDuty100 = mainDuty100 - MAX_DUTY_CHANGE100;
	}
	// Don't set to a negative duty cycle
	if (newDuty100 < 0) {
		newDuty100 = MIN_DUTY100;
	}

	setDutyCycle100(MAIN_ROTOR, newDuty100);

	// If we are at saturation (5% and we want to go lower, or 95%
	// and we want to go higher), don't integrate any further
	if ((mainDuty100 <= MIN_DUTY100 && error < 0) ||
			(mainDuty100 >= MAX_DUTY100 && error > 0)) {
		return;
	}

	// delta t = 0.5 seconds
	errorIntegrated += error / 2;
}

/**
 * Adjusts the PWM duty cycle of the tail rotor to control the yaw.
 */
void yawControl (void) {
	if (!initialised) {
		return;
	}
	static signed long errorIntegrated = 0;
	unsigned int tailDuty100 = getDutyCycle100(TAIL_ROTOR);
	int error = _desiredYaw100 - _yaw100;

	// bias = 1500; Kp = 1/4; Ki = 1/4
	int newDuty100 = 1500 + (error / 4) + (errorIntegrated / 4);

	// Don't allow duty cycle to change too much at once
	if (newDuty100 > (signed int)tailDuty100 + MAX_DUTY_CHANGE100) {
		newDuty100 = tailDuty100 + MAX_DUTY_CHANGE100;
	} else if (newDuty100 < (signed int)tailDuty100 - MAX_DUTY_CHANGE100) {
		newDuty100 = tailDuty100 - MAX_DUTY_CHANGE100;
	}
	// Don't set to a negative duty cycle
	if (newDuty100 < 0) {
		newDuty100 = MIN_DUTY100;
	}

	setDutyCycle100(TAIL_ROTOR, newDuty100);

	// If we are at saturation (5% and we want to go lower, or 95%
	// and we want to go higher), don't integrate any further
	if ((tailDuty100 <= MIN_DUTY100 && error < 0) ||
			(tailDuty100 >= MAX_DUTY100 && error > 0)) {
		return;
	}

	// delta t = 0.5 seconds
	errorIntegrated += error / 2;
}
