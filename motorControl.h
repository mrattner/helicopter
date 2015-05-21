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
#define MAIN_INITIAL_DUTY100 1000 // Initial duty cycle
#define TAIL_INITIAL_DUTY100 500 // Initial duty cycle

/*
 * Static variables
 */
static int initialised = 0;

/**
 * Initialise the PWM generators.
 * PWM generator 0: Controls PWM1 (Main rotor)
 * PWM generator 2: Controls PWM4 (Tail rotor)
 */
void initPWMchan (void);

/**
 * Turn off the motors.
 */
void powerDown (void);

/**
 * Turns on the motors and sets duty cycle of both to the initial amount.
 */
void powerUp (void);

/**
 * Sets the PWM duty cycle to be the duty cycle %. Has built in
 * safety at 5% or 95% if dutyCycle set above 95% or below 5%.
 * @param rotor Either MAIN_ROTOR or TAIL_ROTOR
 * @param dutyCycle100 The desired duty cycle, * 100
 */
void setDutyCycle100 (unsigned long rotor, unsigned int dutyCycle100);

/**
 * Get the current duty cycle of the specified rotor PWM.
 * @param rotor Either MAIN_ROTOR or TAIL_ROTOR
 * @return The current duty cycle % of that rotor's PWM channel
 */
unsigned int getDutyCycle100 (unsigned long rotor);

/**
 * Adjusts the PWM duty cycle of the main rotor to control the altitude.
 */
void altitudeControl (void);

/**
 * Adjusts the PWM duty cycle of the tail rotor to control the yaw.
 */
void yawControl (void);


#endif /* MOTORCONTROL_H_ */
