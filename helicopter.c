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

/*
 * Constants
 */
// Sample rate in Hz
#define SYSTICK_RATE_HZ 2000

// How many samples over which to average the altitude
#define BUF_SIZE 20

// The three display screens
#define STATUS_DISPLAY 0
#define YAW_DISPLAY 1
#define ALTITUDE_DISPLAY 2

// How many degrees * 100 the helicopter rotates per step
#define YAW_DEG_STEP_100 160

#define PWM_RATE_HZ 150
#define PWM_DEF_DUTY 95 // Initial duty cycle

/*
 * Static variables (shared within this file)
 */

static circBuf_t altitudeBuffer; // Altitude buffer

// Minimum and maximum altitude values. Higher number = lower altitude
static int minAltitude = -1;
static int maxAltitude = -1;

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
 * Handler for the ADC conversion complete interrupt.
 */
void ADCIntHandler (void) {
	unsigned long ulValue;

	// Get the sample from ADC0 and store in ulValue.
	ADCSequenceDataGet(ADC0_BASE, 3, &ulValue);

	// Place ulValue in the altitude buffer.
	writeCircBuf(&altitudeBuffer, ulValue);

	// Clear the ADC interrupt.
	ADCIntClear(ADC0_BASE, 3);

	// On our first ADC sample, store the min and max altitude
	if (minAltitude == -1) {
		minAltitude = ulValue;
		// 1023 is the max quantisation value (3.0 V). Divide
		// this by 3 to get the quantisation value for 1.0 V
		maxAltitude = ulValue - (1023 / 3);
	}
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
 * Initialise the analogue-to-digital converter peripheral.
 */
void initADC (void) {
	// Enable the ADC0 peripheral
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

	// Enable sample sequence 3 with a processor signal trigger.
	// Sequence 3 will do a single sample when the processor
	// sends a signal to start the conversion (via the ADCProcessorTrigger
	// function).
	ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);

	// Configure step 0 on sequence 3. Sample channel 0 in the default
	// mode (single-ended) and configure the interrupt flag (IE) to be set
	// when the sample is done. This is the last conversion on sequence
	// 3 (END). We are using sequence 3 because we are only using a single
	// step (step 0).
	ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 | ADC_CTL_IE |
			ADC_CTL_END);

	// Enable sample sequence 3
	ADCSequenceEnable(ADC0_BASE, 3);

	// Register the interrupt handler
	ADCIntRegister(ADC0_BASE, 3, ADCIntHandler);

	// Enable interrupts for ADC0 sequence 3
	ADCIntEnable(ADC0_BASE, 3);
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
 * Converts the altitude measurement to a percentage.
 * @param ulMeanA The current altitude measurement
 * @return The altitude as a percentage
 */
int altitudePercent (unsigned long long ulMeanA) {
	// Find the difference between minimum and maximum altitude
	int diff = minAltitude - maxAltitude;

	// Find the difference between the measurement and MIN_ALTITUDE.
	// Divide MAX_ALTITUDE by this number and multiply by 100 to get
	// the altitude as a percentage.
	return (minAltitude - ulMeanA) * 100 / diff;
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
	initCircBuf(&altitudeBuffer, BUF_SIZE);
	initSysTick();
	initRefPin();
	initPins();
	initADC();
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
	unsigned long long sumA;
	int i;

	initMain();

	while (1) {
		// Background task: calculate the mean of the values in the
		// altitude buffer.
		sumA = 0ull;
		for (i = 0; i < altitudeBuffer.size; i++) {
			sumA += readCircBuf(&altitudeBuffer);
		}

		_avgAltitude = altitudePercent(sumA / altitudeBuffer.size);
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

