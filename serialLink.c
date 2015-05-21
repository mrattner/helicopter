/*
 * uart.c
 *
 * Serial link via UART for outputting status information.
 * Based on HeliSerialTest.c by P. J. Bones
 *
 * Author: J. Shaw and M. Rattner
 */

#include "serialLink.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"

/**
 * Initialise UART0 with 8 bits, 1 stop bit, and no parity.
 */
void initConsole (void) {
	// Enable GPIO port A which is used for UART0 pins
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	// Select the alternate (UART) function for these pins
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), BAUD_RATE,
			UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
	UARTFIFOEnable(UART0_BASE);
	UARTEnable(UART0_BASE);
}

/**
 * Transmit a string via UART0.
 * @param pucBuffer String of characters to send
 */
void UARTSend (char* pucBuffer) {
	// Loop while there are more characters to send
	while (*pucBuffer) {
		// Write the next character to the UART Tx FIFO
		UARTCharPut(UART0_BASE, *pucBuffer);
		pucBuffer++;
	}
}
