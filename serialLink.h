#ifndef SERIALLINK_H_
#define SERIALLINK_H_

/*
 * uart.h
 *
 * Serial link via UART for outputting status information.
 * Based on HeliSerialTest.c by P. J. Bones
 *
 * Author: J. Shaw and M. Rattner
 */

/*
 * Constants
 */
#define BAUD_RATE 9600ul

/**
 * Initialise UART0 with 8 bits, 1 stop bit, and no parity.
 */
void initConsole (void);

/**
 * Transmit a string via UART0.
 * @param pucBuffer String of characters to send
 */
void UARTSend (char* pucBuffer);

/**
 * Construct a status string and send via UART0.
 */
void sendStatus (void);


#endif /* SERIALLINK_H_ */
