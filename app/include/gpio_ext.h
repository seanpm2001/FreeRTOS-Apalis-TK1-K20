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

int gpio_registers(dspi_transfer_t *spi_transfer);

#endif /* SOURCE_GPIO_EXT_H_ */
