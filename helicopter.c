/******************************************************************************
*
* helicopter.c - Program to control a heli-rig.
*
* Author: J. Shaw and M. Rattner
**/
#include "globals.h"
#include "display.h"
#include "altitude.h"
#include "buttonSet.h"
#include "buttonCheck.h"
#include "motorControl.h"
#include "serialLink.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"

#include "driverlib/adc.h"
#include "driverlib/pwm.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/debug.h"
#include "drivers/rit128x96x4.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

/*
 * Constants
 */
// Number of degrees * 100 per slot on the yaw encoder
#define YAW_DEG_STEP_100 160 // TODO: Should this be 320?

/**
 * The interrupt handler called when the SysTick counter reaches 0.
 */
void SysTickIntHandler (void) {
	static int prevA = -1;
	unsigned long pinRead;
	unsigned int yawA, yawB;

	// Trigger an ADC conversion
	ADCProcessorTrigger(ADC_BASE, 3);

	// Update the status of the buttons
	updateButtons();

	// Read the GPIO pins
	pinRead = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_5 | GPIO_PIN_7);
	yawA = pinRead & GPIO_PIN_5;
	yawB = pinRead & GPIO_PIN_7;

	if (!prevA && yawA) {
		if (yawB) {
			_yaw100 += YAW_DEG_STEP_100;
		} else {
			_yaw100 -= YAW_DEG_STEP_100;
		}
	} else if (prevA && !yawA) {
		if (yawB) {
			_yaw100 -= YAW_DEG_STEP_100;
		} else {
			_yaw100 += YAW_DEG_STEP_100;
		}
	}
	prevA = yawA;
}

/**
 * Initialise the GPIO ports and pins.
 * Input on PF5 and PF7
 * Output on PF2 and PD1
 */
void initPins (void) {
	// Enable the ports that will be used (Port D and Port F)
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	// Configure input pins: PF5 (Pin 27) and PF7 (Pin 29)
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_5|GPIO_PIN_7);
	// Use weak pull-up on input pins
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_5|GPIO_PIN_7, GPIO_STRENGTH_2MA,
	   GPIO_PIN_TYPE_STD_WPU);

	// Initialise PD1/PWM1 (Pin 53) for PWM output
	GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_1);
	// Initialise PF2/PWM4 (Pin 22) for PWM output
	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2);
}

/**
 * Initialise the system clock and SysTick.
 */
void initSysTick (void) {
	// Set the system clock rate at maximum / 10 (20 MHz).
	// Note that the PLL must be used in order to use the ADC (Peripheral docs,
	// section 19.1). ADC rate = 8 MHz / 10.
	SysCtlClockSet(SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
				   SYSCTL_XTAL_8MHZ);

	// Set up the period for the SysTick timer. The SysTick timer period is
	// set as a function of the system clock.
	SysTickPeriodSet(SysCtlClockGet() / SYSTICK_RATE_HZ);

	// Register the interrupt handler
	SysTickIntRegister(SysTickIntHandler);

	// Enable interrupt and device
	SysTickIntEnable();
	SysTickEnable();
}

/**
 * Calls initialisation functions.
 */
void initMain (void) {
	SysCtlPeripheralReset(SYSCTL_PERIPH_PWM);
	SysCtlPeripheralReset(SYSCTL_PERIPH_GPIOD);
	SysCtlPeripheralReset(SYSCTL_PERIPH_GPIOB);
	SysCtlPeripheralReset(SYSCTL_PERIPH_GPIOF);
	SysCtlPeripheralReset(SYSCTL_PERIPH_ADC0);
	SysCtlDelay(2);

	initSysTick();
	initPins();
	initPWMchan();
	initAltitudeMonitoring();
	initButtons();
	//initDisplay();
	//initConsole();
	SysCtlDelay(2);


	// Enable interrupts to the processor.
	IntMasterEnable();
}

/**
 * Main program loop.
 */
int main (void) {
	initMain();

	static unsigned long count = 0;

	while (1) {
		if (_heliState == HELI_STARTING) {
			powerUp();
		}

		if (_heliState == HELI_STOPPING) {
			powerDown();
		}

		// Calculate the mean of the values in the altitude buffer
		calcAvgAltitude();

		// Check for button presses and perform associated actions
		checkButtons();

		// Adjust altitude and yaw to desired values
		if (_heliState != HELI_OFF && count % 1000 == 0) {
			altitudeControl();
			count = 0;
		}
		if (_heliState != HELI_OFF && count % 2000 == 0) {
			yawControl();
		}
		count++;


		// Call the display functions
//		displayAltitude();
//		displayYaw();
//		displayPWMStatus(getDutyCycle100(MAIN_ROTOR), getDutyCycle100(TAIL_ROTOR));
	}
}
