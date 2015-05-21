#ifndef GLOBALS_H_
#define GLOBALS_H_

/**
 * globals.h
 *
 * Contains the global variables and constants used by the
 * helicopter control program.
 */

/* Constants */
// Sample rate in Hz
#define SYSTICK_RATE_HZ 6000

// Percent the altitude should change when buttons are pressed
#define ALTITUDE_STEP 10
// Degrees * 100 the yaw should change when buttons are pressed
#define YAW_STEP_100 1500

enum heli_state { HELI_OFF = 0, HELI_STARTING, HELI_ON, HELI_STOPPING };

/* Global Variables */

// Actual values - set by interrupts that monitor the helicopter
extern volatile int _yaw100; // Degrees * 100
extern volatile int _avgAltitude; // Percent

// Desired values - set by button presses
extern int _desiredYaw100; // Degrees * 100
extern int _desiredAltitude; // Percent

// State of the helicopter
extern int _heliState;


#endif /* GLOBALS_H_ */
