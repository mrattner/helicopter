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
 * Display the duty cycle of the PWM generators.
 * @param mainDuty Duty cycle of the main rotor
 * @param tailDuty Duty cycle of the tail rotor
 */
void displayPWMStatus (unsigned int mainDuty, unsigned int tailDuty);
