# Helicopter Rig Control

Written by Marcy Rattner and Joshua Shaw for ENCE361 
Embedded Systems 1, University of Canterbury, 2015

# Assignment

The assignment was to produce a program to control a model helicopter 
using the Texas Instruments Stellaris LM3S1968 microcontroller.

The Stellaris is programmed with a small real-time kernel and 
interrupt-driven control program designed to control the lift off, hover, 
heading while hovering, and landing a small model helicopter mounted
on a telescopic post. The power to two motors, one for the main rotor and 
the other for the stabilising tail rotor, is controlled by pulse width 
modulated (PWM) outputs.

# Build requirements

Because this program is designed to run on a particular Stellaris 
microcontroller, students built their programs in Code Composer Studio 
IDE, a version of Eclipse produced by Texas Instruments. The TI ARM 
compiler is recommended because it requires less setup in CCS. We have not 
attempted to build it outside of the CCS environment.

The TI Stellaris libraries are required to be linked to the project in order 
to build it. Drivers may be required to connect the Stellaris board to 
a computer via USB.

# Program requirements

* Decode the 2-channel quadrature signal for the helicopter yaw _without_ 
using the Stellaris's built-in quadrature encoding unit
* Sample and convert the analogue signal for the helicopter altitude using 
the Stellaris ADC unit, averaging over several samples to reduce error
* Generate two output PWM signals: one for the main rotor and one for the 
tail rotor
* Toggle the motors on and off using the SELECT button input
* Use UP, DOWN, LEFT, and RIGHT buttons to control the altitude and yaw 
of the helicopter, ignoring further input from a button when the helicopter 
has reached a limit of altitude or yaw
* Press the RESET button to reset the system
* Implement a real-time foreground/background kernel operating on a 
round-robin basis to monitor the helicopter's altitude and yaw and respond 
to inputs
* Show the current altitude and yaw and the duty cycle of each PWM output 
signal on the Stellaris OLED display
* Use the UART serial link to output information on the status of the 
helicopter to a remote host at regular intervals

# The helicopter rig

The helicopter is mounted on a light telescopic post.

Altitude is monitored by means of an optical distance transducer and simple
electronic interface which produces an analogue signal between ~2V
(down) and ~1V (up), corresponding to the approximately 70 mm
altitude range.

The yaw is monitored by means of a quadrature optical angle encoder 
which generates two binary signals at voltage levels
compatible with the Stellaris. There are 112 slots around the
circumference of the angle encoder (i.e., 112 over 360 degrees). The
yaw movement is limited to approximately ± 165 degrees either side of
centre.
