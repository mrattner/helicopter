#ifndef GLOBALS_H_
#define GLOBALS_H_

/**
 * globals.h
 *
 * Contains the global variables and constants used by the
 * helicopter control program.
 */

/* Constants */

// Percent the altitude should change when buttons are pressed
#define ALTITUDE_STEP 10
// Degrees the yaw should change when buttons are pressed
#define YAW_STEP 15

/* Global Variables */

// Actual values - set by interrupts that monitor the helicopter
extern volatile int yaw = 0; // Degrees
extern volatile int avgAltitude = 0; // Percent

// Desired values - set by button presses
extern int desiredYaw = 0; // Degrees
extern int desiredAltitude = 0; // Percent

// State of the helicopter
extern enum heli_state = {OFF, STARTING, ON, STOPPING};
extern int heliState = OFF;

extern int currentDisplay = 0; // Start with Status display


#endif /* GLOBALS_H_ */
