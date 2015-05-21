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

/**
 * Construct a status string and send via UART0.
 */
void sendStatus (void) {
	char string[160];
	char* heliMode;

	switch (_heliState) {
	case HELI_OFF:
		heliMode = "Landed";
		break;
	case HELI_STARTING:
		heliMode = "Takeoff";
		break;
	case HELI_ON:
		heliMode = "Flying";
		break;
	case HELI_STOPPING:
		heliMode = "Landing";
		break;
	default:
		heliMode = "Invalid";
		break;
	}

	snprintf(string, 23, "Desired yaw: %d deg\r\n",
			(_desiredYaw100 + 50) / 100);
	snprintf(string + strlen(string), 22, "Actual yaw: %d deg\r\n",
			(_yaw100 + 50) / 100);
	snprintf(string + strlen(string), 24, "Desired altitude: %d%%\r\n",
			_desiredAltitude);
	snprintf(string + strlen(string), 23, "Actual altitude: %d%%\r\n",
			_avgAltitude);
	snprintf(string + strlen(string), 18, "Main rotor: %d%%\r\n",
			(getDutyCycle100(MAIN_ROTOR) + 50) / 100);
	snprintf(string + strlen(string), 18, "Tail rotor: %d%%\r\n",
			(getDutyCycle100(TAIL_ROTOR) + 50) / 100);
	snprintf(string + strlen(string), 22, "Heli mode: %s\r\n\r\n",
			heliMode);

	UARTSend(string);
}
