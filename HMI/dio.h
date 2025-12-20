
/******************************************************************************
 * File: dio.h
 * Module: DIO (Digital Input/Output)
 * Description: Header file for TM4C123GH6PM GPIO Driver
 * Author: Ahmedhh
 * Date: November 22, 2025
 ******************************************************************************/

#ifndef DIO_H_
#define DIO_H_

#include <stdint.h>

/******************************************************************************
 *                              Definitions                                    *
 ******************************************************************************/

/*
 * Port Definitions
 * Used to select the GPIO port in DIO functions.
 */
#define PORTA       0
#define PORTB       1
#define PORTC       2
#define PORTD       3
#define PORTE       4
#define PORTF       5

/*
 * Pin Definitions
 * Used to select the GPIO pin in DIO functions.
 */
#define PIN0        0
#define PIN1        1
#define PIN2        2
#define PIN3        3
#define PIN4        4
#define PIN5        5
#define PIN6        6
#define PIN7        7

/*
 * Direction Definitions
 * Used to set pin direction as input or output.
 */
#define INPUT       0
#define OUTPUT      1

/*
 * Pin Level Definitions
 * Used to set or read pin state (HIGH/LOW).
 */
#define LOW         0
#define HIGH        1

/*
 * Enable/Disable Options
 * Used for enabling/disabling pull-up/pull-down resistors.
 */
#define ENABLE      1
#define DISABLE     0


/******************************************************************************
 * Function Prototypes
 * API for GPIO pin configuration and control.
 ******************************************************************************/


/*
 * DIO_Init
 * Initializes a GPIO pin as input or output.
 */
void DIO_Init(uint8_t port, uint8_t pin, uint8_t direction);

/*
 * DIO_WritePin
 * Writes a value (HIGH/LOW) to a GPIO pin.
 */
void DIO_WritePin(uint8_t port, uint8_t pin, uint8_t value);

/*
 * DIO_ReadPin
 * Reads the value of a GPIO pin (returns HIGH/LOW).
 */
uint8_t DIO_ReadPin(uint8_t port, uint8_t pin);

/*
 * DIO_TogglePin
 * Toggles the value of a GPIO pin.
 */
void DIO_TogglePin(uint8_t port, uint8_t pin);

/*
 * DIO_SetPUR
 * Enables or disables the internal pull-up resistor on a pin.
 */
void DIO_SetPUR(uint8_t port, uint8_t pin, uint8_t enable);

/*
 * DIO_SetPDR
 * Enables or disables the internal pull-down resistor on a pin.
 */
void DIO_SetPDR(uint8_t port, uint8_t pin, uint8_t enable);

#endif /* DIO_H_ */
