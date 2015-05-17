/******************************************************************************
*
* buttonCheck.h - Button control logic.
*
* Author: J. Shaw and M. Rattner
**/

#ifndef BUTTONCHECK_H_
#define BUTTONCHECK_H_

/**
 * Initialise the buttons.
 */
void initButtons (void);

/**
 * Checks each button for a press and performs an associated action:
 *  UP: Increases desired altitude
 *  DOWN: Decreases desired altitude
 *  LEFT: Increments yaw counterclockwise
 *  RIGHT: Increments yaw clockwise
 *  SELECT: Starts up or lands the helicopter
 *  RESET: Perform a "soft" system reset via SysCtl
 */
void checkButtons (void);


#endif /* BUTTONCHECK_H_ */
