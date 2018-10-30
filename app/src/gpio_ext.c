/*
 * gpio_ext.c
 *
 */

#include "gpio_ext.h"
#include "com_task.h"
#include "errno.h"

extern const struct gpio_id *gpio_list;


static inline int port_type_to_int(PORT_Type *port)
{
	switch ((int) port) {
	case PORTA_BASE:
		return 0;
	case PORTB_BASE:
		return 1;
	case PORTC_BASE:
		return 2;
	case PORTD_BASE:
		return 3;
	case PORTE_BASE:
		return 4;
	default:
		return -EINVAL;
	}
}

/* returns GPIO index in gpio_list table and -EINVAL
 * if there is no entry for this gpio.
 */
static int is_gpio_valid(uint8_t pin)
{
	uint16_t i;
	int temp;
	if (pin == 0xFF)
		return -EINVAL;

	for (i = 0; i < sizeof(gpio_list)/sizeof(struct gpio_id); i++){
		temp = port_type_to_int(gpio_list[i].port) * 32;
		temp += gpio_list[i].pin;
		if ( temp == pin ) {
			return i;
		}
		if (temp > pin)
			return -EINVAL;
	}

	return -EINVAL;
}

static int set_gpio_status(uint8_t status, int index)
{
	gpio_pin_config_t gpio_config;

	gpio_config.pinDirection = (status & APALIS_TK1_K20_GPIO_STA_OE) ? kGPIO_DigitalOutput : kGPIO_DigitalInput;
	gpio_config.outputLogic = (status & APALIS_TK1_K20_GPIO_STA_VAL);

	if (index >= 0)
		GPIO_PinInit(gpio_list[index].gpio, gpio_list[index].pin, &gpio_config);
	else
		return index;

	return 0;
}


static uint8_t get_gpio_status(int index)
{
	uint8_t status;
	GPIO_Type *base;
	uint32_t gpio_pin;

	if (index == -EINVAL)
		return 0xFF;
	base = gpio_list[index].gpio;
	gpio_pin = gpio_list[index].pin;

	if (((base->PDDR) >> gpio_pin) & 0x01U) {
		status = APALIS_TK1_K20_GPIO_STA_OE;
		status += (((base->PDOR) >> gpio_pin) & 0x01U) ? APALIS_TK1_K20_GPIO_STA_VAL : 0x00;
	} else {
		status = 0x00;
		status += (((base->PDIR) >> gpio_pin) & 0x01U) ? APALIS_TK1_K20_GPIO_STA_VAL : 0x00;
	}

	return status;
}

int gpio_registers(dspi_transfer_t *spi_transfer)
{
	uint8_t *rx_buf = spi_transfer->rxData;
	int index;

	if (rx_buf[0] == APALIS_TK1_K20_WRITE_INST) {
		switch (rx_buf[1]) {
		case APALIS_TK1_K20_GPIOREG:
			return -ENOENT;
			break;
		case APALIS_TK1_K20_GPIO_NO:
			index = is_gpio_valid(rx_buf[2]);
			if (index >= 0){
				registers[APALIS_TK1_K20_GPIO_NO]= rx_buf[2];
				registers[APALIS_TK1_K20_GPIO_STA] = get_gpio_status(index);
				return 1;
			} else {
				registers[APALIS_TK1_K20_GPIO_NO] = 0xFF;
				return -ENOENT;
			}
			break;
		case APALIS_TK1_K20_GPIO_STA:
			index = is_gpio_valid(registers[APALIS_TK1_K20_GPIO_NO]);
			if (index >= 0){
			set_gpio_status(rx_buf[2], index);
			registers[APALIS_TK1_K20_GPIO_STA] = get_gpio_status(index);
			return 1;
			} else {
				return -ENOENT;
			}
			break;
		default:
			return -ENOENT;
		}
	}
	return -ENOENT;
}




