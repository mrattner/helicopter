/**
 * altitude.c
 *
 * The altitude module controls the ADC monitoring of altitude.
 *
 * Author: J. Shaw and M. Rattner
 */

#include "globals.h"
#include "altitude.h"
#include "circBuf.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#include "driverlib/adc.h"
#include "driverlib/sysctl.h"

/*
 * Static variables (shared within this file)
 */

// Altitude buffer
static circBuf_t altitudeBuffer;

// Minimum and maximum altitude values. Higher number = lower altitude
static long minAltitude = -1;
static long maxAltitude = -1;

// Number of ADC reads during which the heli has been landed
static unsigned long landedCount = 0;

/**
 * Handler for the ADC conversion complete interrupt.
 */
void ADCIntHandler (void) {
	unsigned long ulValue;

	// Clear the ADC interrupt
	ADCIntClear(ADC0_BASE, 3);

	// Get the sample from ADC0 and store in ulValue
	ADCSequenceDataGet(ADC0_BASE, 3, &ulValue);

	// Ignore invalid values
	if (ulValue > 1023) {
		return;
	}

	// Place ulValue in the altitude buffer
	writeCircBuf(&altitudeBuffer, ulValue);

	// Keep track of how long the heli has been landed
	if (_heliState == HELI_OFF) {
		landedCount++;
	} else {
		landedCount = 0;
	}

	// Get an initial value for max. and min. altitude
	if (minAltitude == -1) {
		minAltitude = ulValue;
		// Max. altitude is a lower voltage level than min. altitude
		maxAltitude = minAltitude - V_DIFF_DISCRETE;
	}
}

/**
 * Initialise the analogue-to-digital converter peripheral.
 */
void initADC (void) {
	// Initialise the altitude buffer
	initCircBuf(&altitudeBuffer, BUF_SIZE);

	// Enable the ADC0 peripheral
	SysCtlPeripheralReset(SYSCTL_PERIPH_ADC0);
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
 * Calculates the average altitude percentage from the values in
 * the altitude buffer and updates the altitude global variable.
 */
void calcAvgAltitude (void) {
	unsigned long long sumA;
	unsigned long meanA;
	int i;

	sumA = 0ull;
	for (i = 0; i < altitudeBuffer.size; i++) {
		sumA += readCircBuf(&altitudeBuffer);
	}

	meanA = sumA / altitudeBuffer.size;

	// Continually recalibrate min. and max. altitude while
	// the helicopter is known to be landed
	if (landedCount > altitudeBuffer.size && meanA > V_DIFF_DISCRETE) {
		minAltitude = meanA;
		// Max. altitude is a lower voltage level than min. altitude
		maxAltitude = minAltitude - V_DIFF_DISCRETE;
	}

	// Find the difference between the measurement and the minimum
	// altitude voltage level. Divide this number by the difference
	// between the max. and min. altitude levels and multiply by 100
	// to get the altitude as a percentage.
	_avgAltitude = (minAltitude - (signed long)meanA) * 100 /
			(minAltitude - maxAltitude);
}
