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
#include "driverlib/timer.h"
#include "driverlib/debug.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

/*
 * Constants
 */
// Number of degrees * 100 per slot on the yaw encoder
#define YAW_DEG_STEP_100 160

enum tasks {ALTITUDE_CTRL = 0,
	YAW_CTRL = 1,
	DISPLAY = 2,
	BUTTONS = 3,
	MESSAGE = 4,
	BUFFER_AVG = 5};

typedef struct {
	unsigned long lastExecuted; // Timer count when it last occurred
	unsigned int waitTimeUsec; // Time between executions in microseconds
	unsigned long waitTicks; // Number of systicks between executions
	unsigned int blocked; // 0 if not blocked, 1 if blocked
} backgroundTask_t;

// Array of background tasks
static backgroundTask_t tasks[6];

// How many timer ticks have occurred. Will take over 24 days to overflow
volatile static unsigned long timerTicks = 0;

/**
 * Defines the time to wait between execution of background tasks.
 */
void defineTasks (void) {
	// 1000 us = 1 ms; 1,000,000 us = 1 s
	tasks[DISPLAY].waitTimeUsec = 250000;
	tasks[MESSAGE].waitTimeUsec = 6000000;

	tasks[BUTTONS].waitTimeUsec = 500;
	tasks[BUFFER_AVG].waitTimeUsec = 500;

	tasks[ALTITUDE_CTRL].waitTimeUsec = 500000;
	tasks[YAW_CTRL].waitTimeUsec = 500000;

	unsigned long usecPerTick = 1000000 / SYSTICK_RATE_HZ;
	int i;
	for (i = 0; i < 6; i++) {
		tasks[i].lastExecuted = 0ul;
		tasks[i].waitTicks =
				tasks[i].waitTimeUsec / usecPerTick;
		tasks[i].blocked = 1; // Block all tasks initially
	}
}

/**
 * Determines whether it is time to perform a background task.
 * @param task One of the enumerated background tasks
 * @return 0 if not enough time has passed since the last execution
 * of the task (or it is blocked); 1 if the task should be executed
 * again now
 */
unsigned int isTimeFor(int task) {
	if (tasks[task].blocked == 1) {
		return 0;
	}
	unsigned long diff = timerTicks - tasks[task].lastExecuted;
	return (diff >= tasks[task].waitTicks) ? 1 : 0;
}

/**
 * The interrupt handler called when the timer reaches 0.
 */
void TimerIntHandler (void) {
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	timerTicks++;
	SysCtlDelay(5); // Allow enough time for the interrupt to be cleared
}

/**
 * The interrupt handler called when the SysTick counter reaches 0.
 */
void SysTickIntHandler (void) {
	static int prevA = -1;
	unsigned long pinRead;
	unsigned int yawA, yawB;

	// Trigger an ADC conversion
	ADCProcessorTrigger(ADC_BASE, 3);
	tasks[BUFFER_AVG].blocked = 0; // Can take average after new value stored

	// Update the status of the buttons
	updateButtons();
	tasks[BUTTONS].blocked = 0; // Can respond to new button press now

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
	tasks[YAW_CTRL].blocked = 0; // Can control yaw after each measurement
}

/**
 * Initialise the GPIO ports and pins.
 * Input on PF5 and PF7
 * Output on PF2 and PD1
 */
void initPins (void) {
	// Enable the ports that will be used (Port D and Port F)
	SysCtlPeripheralReset(SYSCTL_PERIPH_GPIOD);
	SysCtlPeripheralReset(SYSCTL_PERIPH_GPIOF);
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
 * Initialise SysTick interrupts. Must be called after system clock set.
 */
void initSysTick (void) {
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
 * Configure and enable the timer module. Must be called after
 * system clock set.
 */
void initTimer (void) {
	SysCtlPeripheralReset(SYSCTL_PERIPH_TIMER0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

	// Configure a 32-bit periodic timer
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

	// Define the maximum value that the timer counts down from.
	// If it counts down from the system clock rate, it will take 1 second
	TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet() / SYSTICK_RATE_HZ);

	// Configure interrupt handler to be called when timer reaches 0
	TimerIntRegister(TIMER0_BASE, TIMER_A, TimerIntHandler);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	TimerEnable(TIMER0_BASE, TIMER_A);
}

/**
 * Construct a status string and send via UART0.
 */
void sendStatus (void) {
	char string[170];
	char* heliMode;

	switch (_heliState) {
	case HELI_OFF:
		heliMode = "Landed";
		break;
	case HELI_STARTING:
		heliMode = "Takeoff";
		break;
	case HELI_ON:
		heliMode = "Flying";
		break;
	case HELI_STOPPING:
		heliMode = "Landing";
		break;
	default:
		heliMode = "Invalid";
		break;
	}

	snprintf(string, 23, "Desired yaw: %d deg \n",
			(_desiredYaw100 + 50) / 100);
	snprintf(string + strlen(string), 22, "Actual yaw: %d deg \n",
			(_yaw100 + 50) / 100);
	snprintf(string + strlen(string), 24, "Desired altitude: %d%% \n",
			_desiredAltitude);
	snprintf(string + strlen(string), 33, "Actual altitude: %d%% \n",
			_avgAltitude);
	snprintf(string + strlen(string), 18, "Main rotor: %d%% \n",
			(getDutyCycle100(MAIN_ROTOR) + 50) / 100);
	snprintf(string + strlen(string), 18, "Tail rotor: %d%% \n",
			(getDutyCycle100(TAIL_ROTOR) + 50) / 100);
	snprintf(string + strlen(string), 22, "Heli mode: %s \n\n",
			heliMode);

	UARTSend(string);
}

/**
 * Calls initialisation functions.
 */
void initMain (void) {
	// Set the system clock rate at maximum / 10 (20 MHz).
	// Note that the PLL must be used in order to use the ADC (Peripheral docs,
	// section 19.1). ADC rate = 8 MHz / 10.
	SysCtlClockSet(SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
				   SYSCTL_XTAL_8MHZ);

	initConsole();
	tasks[MESSAGE].blocked = 0;

	initDisplay();
	tasks[DISPLAY].blocked = 0;

	initPins();
	initADC();
	initButtons(VIRTUAL);
	initPWMchan();
	initSysTick();
	initTimer();

	SysCtlDelay(2);

	// Enable interrupts to the processor.
	IntMasterEnable();
}

/**
 * Main program loop.
 */
int main (void) {
	defineTasks();
	initMain();
	UARTSend("UART is operational.\n\n");

	while (1) {
		if (_heliState == HELI_STARTING) {
			powerUp();
		}

		if (_heliState == HELI_STOPPING) {
			powerDown();
		}

		// Calculate the mean of the values in the altitude buffer
		if (isTimeFor(BUFFER_AVG)) {
			calcAvgAltitude();
			tasks[ALTITUDE_CTRL].blocked = 0; // Can adjust altitude now
			tasks[BUFFER_AVG].blocked = 1; // Block until next measurement
			tasks[BUFFER_AVG].lastExecuted = timerTicks;
		}

		// Check for button presses and perform associated actions
		if (isTimeFor(BUTTONS)) {
			checkButtons();
			tasks[BUTTONS].blocked = 1; // Block until next button update
			tasks[BUTTONS].lastExecuted = timerTicks;
		}

		// Adjust altitude to desired value
		if (_heliState != HELI_OFF && isTimeFor(ALTITUDE_CTRL)) {
			altitudeControl();
			tasks[ALTITUDE_CTRL].blocked = 1; // Block until next average
			tasks[ALTITUDE_CTRL].lastExecuted = timerTicks;
		}

		// Adjust yaw to desired value
		if (_heliState != HELI_OFF && isTimeFor(YAW_CTRL)) {
			yawControl();
			tasks[YAW_CTRL].blocked = 1; // Block until next measurement
			tasks[YAW_CTRL].lastExecuted = timerTicks;
		}

		// Send status message
		if (isTimeFor(MESSAGE)) {
			sendStatus();
			tasks[MESSAGE].lastExecuted = timerTicks;
		}

		// Refresh the display
		if (isTimeFor(DISPLAY)) {
			displayAltitude();
			displayYaw();
			displayPWMStatus(getDutyCycle100(MAIN_ROTOR), getDutyCycle100(TAIL_ROTOR));
			tasks[DISPLAY].lastExecuted = timerTicks;
		}
	}
}
