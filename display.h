/**
 * display.h
 *
 * Display control module for the helicopter program.
 *
 * Author: J. Shaw and Marcy Rattner
 */

/*
 * Constants
 */
// The three display screens
#define STATUS_DISPLAY 0
#define YAW_DISPLAY 1
#define ALTITUDE_DISPLAY 2

/**
 * Initialise the OLED display with an SSI clock frequency of 1 MHz.
 */
void initDisplay (void);

/**
 * Display the altitude of the heli rig. The measured value from
 * the ADC will be ~1-2 V. Decreasing voltage = increasing altitude.
 */
void displayAltitude ();

/**
 * Display the yaw of the heli rig in degrees, relative to start
 * position.
 */
void displayYaw ();

/**
 * Display the status of the PWM generator.
 * @param pulseWidth Current PWM pulse width
 * @param period Current PWM period
 */
void displayStatus (unsigned long pulseWidth, unsigned long period);
