/**
 * display.h
 *
 * Display control module for the helicopter program.
 *
 * Author: J. Shaw and Marcy Rattner
 */

/**
 * Initialise the OLED display with an SSI clock frequency of 200 kHz.
 * Note that this can only be called after serialLink's initConsole()
 * function because initConsole() resets GPIOA.
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
void displayPWMStatus (unsigned int mainDuty100, unsigned int tailDuty100);
