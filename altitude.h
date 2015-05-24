/*
 * altitude.h
 *
 * The altitude module controls the ADC monitoring of altitude.
 *
 * Author: J. Shaw and M. Rattner
 */

#ifndef ALTITUDE_H_
#define ALTITUDE_H_

/*
 * Constants
 */
// How many samples over which to average the altitude
#define BUF_SIZE 20

// Difference in voltage between min. and max. altitude
// expressed in quantisation levels.
// V_DIFF_DISCRETE = (1023 * 0.8 V) / 3.0 V, rounded
#define V_DIFF_DISCRETE 273

/**
 * Handler for the ADC conversion complete interrupt.
 */
void ADCIntHandler (void);

/**
 * Initialise the analogue-to-digital converter peripheral.
 */
void initADC (void);

/**
 * Calculates the average altitude percentage from the values in
 * the altitude buffer and updates the altitude global variable.
 */
void calcAvgAltitude (void);

#endif /* ALTITUDE_H_ */
