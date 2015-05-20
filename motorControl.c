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

/*
 * Constants
 */
// How much to adjust the rotors' duty cycle
#define MAIN_ROTOR_STEP 2
#define TAIL_ROTOR_STEP 2

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
	PWMPulseWidthSet(PWM_BASE, PWM_OUT_1, period * PWM_INITIAL_DUTY / 100);
	// Generator 2
	PWMGenPeriodSet(PWM_BASE, PWM_GEN_2, period);
	PWMPulseWidthSet(PWM_BASE, PWM_OUT_4, period * PWM_INITIAL_DUTY / 100);

	// Enable the PWM output signals.
	PWMOutputState(PWM_BASE, PWM_OUT_1_BIT | PWM_OUT_4_BIT, true);

	// Enable the PWM generators.
	PWMGenEnable(PWM_BASE, PWM_GEN_0);
	PWMGenEnable(PWM_BASE, PWM_GEN_2);

	initialised = 1;
	SysCtlDelay(2);
}

/**
 * Turn off the motors.
 */
void powerDown (void) {
	SysCtlPeripheralDisable(SYSCTL_PERIPH_PWM);
}

/**
 * Turns on the motors and sets duty cycle of both to the initial amount.
 */
void powerUp (void) {
	if (!initialised) {
		initPWMchan();
	}

	setDutyCycle(MAIN_ROTOR, PWM_INITIAL_DUTY);
	setDutyCycle(TAIL_ROTOR, PWM_INITIAL_DUTY);
}

/**
 * Sets the PWM duty cycle to be the duty cycle %. Has built in
 * safety at 5% or 95% if dutyCycle set above 95% or below 5%.
 * @param rotor Either MAIN_ROTOR or TAIL_ROTOR
 * @param dutyCycle The duty cycle %
 */
void setDutyCycle (unsigned long rotor, unsigned int dutyCycle) {
	if (!initialised) {
		return;
	}
	unsigned long period = (rotor == MAIN_ROTOR) ?
				PWMGenPeriodGet(PWM_BASE, PWM_GEN_0) // main rotor PWM
				: PWMGenPeriodGet(PWM_BASE, PWM_GEN_2); // tail rotor PWM

	if (dutyCycle < 5) {
		dutyCycle = 5;
	} else if (dutyCycle > 95) {
		dutyCycle = 95;
	}

	// Set the pulse width according to the desired duty cycle.
	// Round the value instead of truncating when dividing by 100
	PWMPulseWidthSet(PWM_BASE, rotor, (period * PWM_INITIAL_DUTY + 50) / 100);
}

/**
 * Get the current duty cycle of the specified rotor PWM.
 * @param rotor Either MAIN_ROTOR or TAIL_ROTOR
 * @return The current duty cycle % of that rotor's PWM channel
 */
unsigned int getDutyCycle (unsigned long rotor) {
	if (!initialised) {
		return;
	}
	unsigned long currentPulseWidth = PWMPulseWidthGet(PWM_BASE, rotor);
	unsigned long currentPeriod = (rotor == MAIN_ROTOR) ?
			PWMGenPeriodGet(PWM_BASE, PWM_GEN_0) // main rotor PWM
			: PWMGenPeriodGet(PWM_BASE, PWM_GEN_2); // tail rotor PWM
	return (100 * currentPulseWidth + currentPeriod / 2) / currentPeriod;
}

/**
 * Adjusts the PWM duty cycle of the main rotor to control the altitude.
 */
void altitudeControl (void) {
	if (!initialised) {
			return;
	}
	unsigned int currentDutyCycle = getDutyCycle(MAIN_ROTOR);

	if (_avgAltitude < _desiredAltitude) {
		setDutyCycle(MAIN_ROTOR, currentDutyCycle + MAIN_ROTOR_STEP);
	} else if (_avgAltitude > _desiredAltitude) {
		setDutyCycle(MAIN_ROTOR, currentDutyCycle - MAIN_ROTOR_STEP);
	}
}

/**
 * Adjusts the PWM duty cycle of the tail rotor to control the yaw.
 */
void yawControl (void) {
	if (!initialised) {
		return;
	}
	unsigned int currentDutyCycle = getDutyCycle(TAIL_ROTOR);

	if (_yaw < _desiredYaw) {
		setDutyCycle(TAIL_ROTOR, currentDutyCycle + TAIL_ROTOR_STEP);
	} else if (_yaw > _desiredYaw) {
		setDutyCycle(TAIL_ROTOR, currentDutyCycle - TAIL_ROTOR_STEP);
	}
}
