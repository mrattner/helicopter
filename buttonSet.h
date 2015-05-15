#ifndef HELICOPTER_BUTTONSET_H_
#define HELICOPTER_BUTTONSET_H_

// *******************************************************
// 
// buttonSet.h   ** Version 2 **
//
// Support for buttons on the Stellaris LM3S1968 EVK
// P.J. Bones UCECE
// Last modified:  28.2.2013
// 
// *******************************************************

#include "button.h"

// GPIO ports for virtual vs. physical buttons
#define VIRTUAL_PORT GPIO_PORTB_BASE
#define PHYSICAL_PORT GPIO_PORTG_BASE

// Bit fields used to select which buttons are initialised in the
//  button set.  Designed to be ORed in argument to initButSet().
#define UP_B 0X01
#define DOWN_B 0X02
#define LEFT_B 0X04
#define RIGHT_B 0X08
#define SELECT_B 0X10
#define RESET_B 0X20
// Constants to identify buttons (need to be consecutive)
enum butDefs {UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3, SELECT = 4, RESET = 5};
#define NUM_BUTTONS 6


// *******************************************************
// initButSet: Initialise the button instances for the specific 
//  buttons specified.  Argument = bit pattern formed by UP_B,
//  DOWN_B, etc., ORed together.  Should only be called once the 
//  SysCtlClock frequency is set. 'tickRateHz' is the rate of
//  SysTick interrupts (set externally).
void
initButSet (unsigned char buttons, unsigned long port, unsigned int tickRateHz);


// *******************************************************
// updateButtons: Function designed to be called from the SysTick
//  interrupt handler.  It has no return type or argument list. A
//  call to initButSet() is required before the first call to this.
void
updateButtons (void);


// *******************************************************
// checkBut: Checks the specified individual button and returns true 
//  (1) if the button is active and has state BUT_PUSHED (newly pushed) 
//  and modifies state to BUT_IN. Returns false (0) otherwise.
unsigned int
checkBut (unsigned int button);


// *******************************************************
// anyButPushed: Checks the current set of active buttons and returns 
//  true (>0) if any of them have state = BUT_PUSHED. Value returned 
//  is an ORed set of the bits representing the button(s) newly pushed,
//  e.g., if "UP" and "SELECT" have been recently pushed, the 
//  value returned is (UP_B | SELECT_B). Otherwise returns false (0). 
//  State of any pushed button is altered to BUT_IN.
unsigned char
anyButPushed (void);


// *******************************************************
// enableBut: Alters the state of the specified button to BUT_OUT, 
//  if it was previously BUT_INACTIVE, otherwise makes no change.
void
enableBut (unsigned int button);


// *******************************************************
// disableBut: Alters the state of the specified button to 
//  BUT_INACTIVE.
void
disableBut (unsigned int button);


#endif /*HELICOPTER_BUTTONSET_H_*/
