/**
 * globals.c
 *
 * Initialises the global variables used by the helicopter
 * control program.
 */

#include "globals.h"

/* Global Variables */

// Actual values - set by interrupts that monitor the helicopter
volatile int _yaw = 0; // Degrees * 100
volatile int _avgAltitude = 0; // Percent

// Desired values - set by button presses
int _desiredYaw = 0; // Degrees
int _desiredAltitude = 0; // Percent

// State of the helicopter
int _heliState = HELI_OFF;
