/*
 * motorControl.c
 *
 *  Created on: 15/05/2015
 *      Author: sypro
 */

#include "globals.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/pwm.h"
#include "driverlib/gpio.h"


/**
 * Sets the PWM duty cycle to be the duty cycle %. Has built in
 * safety at 2% or 98% if dutyCycle set above 98% or drops below 2%.
 * @param dutyCycle The duty cycle %
 */
void setTailRotorDutyCycle (int dutyCycle) {

	if (dutyCycle < 2) {
		dutyCycle = 2;
	} else if (dutyCycle > 98) {
		dutyCycle = 98;
	}

	// Set the pulse width according to the desired duty cycle.
	// Round the value instead of truncating when dividing by 100
	PWMPulseWidthSet(PWM_BASE, PWM_OUT_4, dutyCycle);
}

/**
 * Sets the PWM duty cycle to be the duty cycle %. Has built in
 * safety at 2% or 98% if dutyCycle set above 98% or drops below 2%.
 * @param dutyCycle The duty cycle %
 */
void setMainRotorDutyCycle (int dutyCycle) {

	if (dutyCycle < 2) {
		dutyCycle = 2;
	} else if (dutyCycle > 98) {
		dutyCycle = 98;
	}

	// Set the pulse width according to the desired duty cycle.
	// Round the value instead of truncating when dividing by 100
	PWMPulseWidthSet(PWM_BASE, PWM_OUT_4, dutyCycle);
}
