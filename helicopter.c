/******************************************************************************
*
* helicopter.c - Program to control a heli-rig.
*
* Author: J. Shaw and M. Rattner
**/
#include "globals.h"
#include "circBuf.h"
#include "buttonSet.h"
#include "button.h"
#include "display.h"
#include "altitude.h"

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

/*
 * Constants
 */
// Sample rate in Hz
#define SYSTICK_RATE_HZ 2000

// Number of degrees * 100 per slot on the yaw encoder
#define YAW_DEG_STEP_100 160

#define PWM_RATE_HZ 150
#define PWM_DEF_DUTY 95 // Initial duty cycle

/**
 * The interrupt handler called when the SysTick counter reaches 0.
 */
void SysTickIntHandler (void) {
	static int prevA = -1;
	unsigned long pinRead;
	unsigned int yawA, yawB;

	// Trigger an ADC conversion
	ADCProcessorTrigger(ADC_BASE, 3);

	// Check for button presses
	updateButtons();

	// Read the GPIO pins
	pinRead = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_5 | GPIO_PIN_7);
	yawA = pinRead & GPIO_PIN_5;
	yawB = pinRead & GPIO_PIN_7;

	if (!prevA && yawA) {
		if (yawB) {
			_yaw += YAW_DEG_STEP_100;
		} else {
			_yaw -= YAW_DEG_STEP_100;
		}
	} else if (prevA && !yawA) {
		if (yawB) {
			_yaw -= YAW_DEG_STEP_100;
		} else {
			_yaw += YAW_DEG_STEP_100;
		}
	}
	prevA = yawA;
}

/**
 * Provide a Vcc source on Pin 56.
 */
void initRefPin (void) {
   // Set Pin 56 (PD0) as a +Vcc low current capacity source
   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
   GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_0);
   GPIOPadConfigSet(GPIO_PORTD_BASE, GPIO_PIN_0, GPIO_STRENGTH_8MA,
      GPIO_PIN_TYPE_STD_WPU);
   GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0, GPIO_PIN_0);
}

/**
 * Initialise the GPIO pins.
 */
void initPins (void) {
	// Enable and configure the port and pins used.
	// Input on PF5: Pin 27 and PF7: Pin 29
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_5|GPIO_PIN_7);
	// Use weak pull-up
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_5|GPIO_PIN_7, GPIO_STRENGTH_2MA,
	   GPIO_PIN_TYPE_STD_WPU);

	// Initialise PF2/PWM4 (Port 22) for PWM output
	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2);
}

/**
 * Initialise the buttons.
 */
void initButtons (void) {
	// Enable GPIO port B for the virtual buttons
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	// The button set being used is UP, DOWN, LEFT, RIGHT,
	// SELECT, and RESET on the virtual port
	initButSet(UP_B | DOWN_B | LEFT_B | RIGHT_B | SELECT_B | RESET_B,
			PHYSICAL_PORT, SYSTICK_RATE_HZ);
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
 * Initialise the PWM generator.
 */
void initPWMchan (void) {
	unsigned long period;

	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM);

	// Compute the PWM period based on the system clock.
	SysCtlPWMClockSet(SYSCTL_PWMDIV_4);
	PWMGenConfigure(PWM_BASE, PWM_GEN_2,
			PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);
	// We set the PWM clock to be 1/4 the system clock, so set the period
	// to be 1/4 the system clock divided by the desired frequency.
	period = SysCtlClockGet() / 4 / PWM_RATE_HZ;
	PWMGenPeriodSet(PWM_BASE, PWM_GEN_2, period);
	PWMPulseWidthSet(PWM_BASE, PWM_OUT_4, period * PWM_DEF_DUTY / 100);

	// Enable the PWM output signal.
	PWMOutputState(PWM_BASE, PWM_OUT_4_BIT, true);

	// Enable the PWM generator.
	PWMGenEnable(PWM_BASE, PWM_GEN_2);
}

/**
 * Sets the PWM duty cycle to be (100 - altitude)%. Holds duty cycle
 * at 5% or 95% if altitude goes above 95% or drops below 5%.
 */
void setDutyCycle () {
	int dutyCycle = 100 - _avgAltitude;

	if (dutyCycle < 5) {
		dutyCycle = 5;
	} else if (dutyCycle > 95) {
		dutyCycle = 95;
	}

	unsigned long period = PWMGenPeriodGet(PWM_BASE, PWM_GEN_2);

	// Set the pulse width according to the desired duty cycle.
	// Round the value instead of truncating when dividing by 100
	PWMPulseWidthSet(PWM_BASE, PWM_OUT_4, (period * dutyCycle + 50) / 100);
}

/**
 * Calls initialisation functions.
 */
void initMain (void) {
	initSysTick();
	initRefPin();
	initPins();
	initAltitudeMonitoring();
	initPWMchan();
	initButtons();
	initDisplay();

	// Enable interrupts to the processor.
	IntMasterEnable();
}

/**
 * Main program loop.
 */
int main (void) {
	initMain();

	while (1) {
		// Background task: calculate the mean of the values in the
		// altitude buffer.
		calcAvgAltitude();

		// Ignore outlying values. Frequently skipping this line indicates
		// that recalibration of the min. and max. altitude is necessary.
		if (_avgAltitude <= 100 && _avgAltitude >= 0) {
			setDutyCycle();
		}

		// Background task: check for button presses and change the
		// current display mode.
		if (checkBut(UP)) {
			RIT128x96x4Clear();	// Clear the OLED display
			switch (_currentDisplay) {
			case ALTITUDE_DISPLAY:
				_currentDisplay = STATUS_DISPLAY;
				break;
			case YAW_DISPLAY:
				_currentDisplay = ALTITUDE_DISPLAY;
				break;
			default:
				_currentDisplay = YAW_DISPLAY;
				break;
			}
		}
		if (checkBut(DOWN)) {
			RIT128x96x4Clear();	// Clear the OLED display
			switch (_currentDisplay) {
			case ALTITUDE_DISPLAY:
				_currentDisplay = YAW_DISPLAY;
				break;
			case YAW_DISPLAY:
				_currentDisplay = STATUS_DISPLAY;
				break;
			default:
				_currentDisplay = ALTITUDE_DISPLAY;
				break;
			}
		}

		// Background task: Call the appropriate display function
		// according to the current display mode.
		switch (_currentDisplay) {
		case YAW_DISPLAY:
			displayYaw();
			break;
		case ALTITUDE_DISPLAY:
			displayAltitude();
			break;
		default:
			displayStatus(PWMPulseWidthGet(PWM_BASE, PWM_OUT_4),
					PWMGenPeriodGet(PWM_BASE, PWM_GEN_2));
		}
	}
}
