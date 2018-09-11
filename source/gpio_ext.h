/*
 * gpio_ext.h
 *
 */

#ifndef SOURCE_GPIO_EXT_H_
#define SOURCE_GPIO_EXT_H_

#include "board.h"
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "fsl_dspi.h"

struct gpio_id{
	PORT_Type *port;
	GPIO_Type *gpio;
	uint32_t pin;
};

struct gpio_id gpio_list [] = {
#ifndef USE_SWO
		{PORTA, GPIOA, 3},
#endif
		{PORTA, GPIOA, 5},
#ifdef TESTER_BUILD
		{PORTA, GPIOA, 12},
		{PORTA, GPIOA, 13},
#endif
		{PORTA, GPIOA, 17},
#ifndef BOARD_USES_ADC
		{PORTB, GPIOB, 0},
		{PORTB, GPIOB, 1},
		{PORTB, GPIOB, 2},
		{PORTB, GPIOB, 3},
#endif
		{PORTB, GPIOB, 10},
		{PORTB, GPIOB, 11},
		{PORTB, GPIOB, 16},
		{PORTB, GPIOB, 17},
		{PORTB, GPIOB, 18},
		{PORTB, GPIOB, 19},
		{PORTC, GPIOC, 0},
		{PORTC, GPIOC, 1},
		{PORTC, GPIOC, 2},
		{PORTC, GPIOC, 3},
		{PORTC, GPIOC, 4},
		{PORTC, GPIOC, 6},
		{PORTC, GPIOC, 7},
#ifdef TESTER_BUILD
		{PORTC, GPIOC, 16},
		{PORTC, GPIOC, 17},
#endif
		{PORTD, GPIOD, 0},
		{PORTD, GPIOD, 1},
		{PORTD, GPIOD, 2},
		{PORTD, GPIOD, 3},
		{PORTD, GPIOD, 4},
		{PORTD, GPIOD, 5},
		{PORTD, GPIOD, 6},
		{PORTD, GPIOD, 7},
		{PORTD, GPIOD, 8},
		{PORTD, GPIOD, 9},
		{PORTD, GPIOD, 11},
		{PORTD, GPIOD, 12},
		{PORTD, GPIOD, 13},
		{PORTD, GPIOD, 14},
		{PORTD, GPIOD, 15},
#if !defined(SDK_DEBUGCONSOLE) || defined(TESTER_BUILD)
		{PORTE, GPIOE, 0},
		{PORTE, GPIOE, 1},
#endif
		{PORTE, GPIOE, 2},
		{PORTE, GPIOE, 3},
		{PORTE, GPIOE, 4},
		{PORTE, GPIOE, 5},
		{PORTE, GPIOE, 24},
		{PORTE, GPIOE, 25}
};

int gpio_registers(dspi_transfer_t *spi_transfer);

#endif /* SOURCE_GPIO_EXT_H_ */
