#ifndef MOTORCONTROL_H_
#define MOTORCONTROL_H_

/*
 * motorControl.h
 *
 * Control module for the main and tail motors of the helicopter.
 *
 * Author: J. Shaw and M. Rattner
 */

/*
 * Constants
 */
#define MAIN_ROTOR PWM_OUT_1
#define TAIL_ROTOR PWM_OUT_4
#define PWM_RATE_HZ 150 // Frequency of the PWM generator
#define PWM_DEF_DUTY 3 // Initial duty cycle

/**
 * Initialise the PWM generators.
 * PWM generator 0: Controls PWM1 (Main rotor)
 * PWM generator 2: Controls PWM4 (Tail rotor)
 */
void initPWMchan (void);

/**
 * Sets the PWM duty cycle to be the duty cycle %. Has built in
 * safety at 5% or 95% if dutyCycle set above 95% or below 5%.
 * @param rotor Either MAIN_ROTOR or TAIL_ROTOR
 * @param dutyCycle The duty cycle %
 */
void setDutyCycle (unsigned long rotor, unsigned int dutyCycle);

/**
 * Get the current duty cycle of the specified rotor PWM.
 * @param rotor Either MAIN_ROTOR or TAIL_ROTOR
 * @return The current duty cycle % of that rotor's PWM channel
 */
unsigned int getDutyCycle (unsigned long rotor);

/**
 * Adjusts the PWM duty cycle of the main rotor to control the altitude.
 */
void altitudeControl (void);

/**
 * Adjusts the PWM duty cycle of the tail rotor to control the yaw.
 */
void yawControl (void);


#endif /* MOTORCONTROL_H_ */
