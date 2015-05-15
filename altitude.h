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

/**
 * Handler for the ADC conversion complete interrupt.
 */
void ADCIntHandler (void);

/**
 * Initialise the analogue-to-digital converter peripheral.
 */
void initAltitudeMonitoring (void);

/**
 * Calculates the average altitude percentage from the values in
 * the altitude buffer and updates the altitude global variable.
 */
void calcAvgAltitude (void);

#endif /* ALTITUDE_H_ */
