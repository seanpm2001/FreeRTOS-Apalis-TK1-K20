/*
 * Copyright (c) 2013 - 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* This is a template file for pins configuration created by New Kinetis SDK 2.x Project Wizard. Enjoy! */

#include "fsl_device_registers.h"
#include "fsl_common.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include "fsl_debug_console.h"
#include "gpio_ext.h"

const struct gpio_id gpio_list[] = {
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

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Initialize all pins used in this example
 */
void BOARD_InitPins(void)
{
	unsigned int i;
	gpio_pin_config_t gpio_out_config = {
			kGPIO_DigitalOutput, 0,
	};
	gpio_pin_config_t gpio_out_hi_config = {
			kGPIO_DigitalOutput, 1,
	};
	gpio_pin_config_t gpio_in_config = {
			kGPIO_DigitalInput,
	};
	port_pin_config_t od_config;
	port_pin_config_t can_tx_config;
	port_pin_config_t in_config;

	CLOCK_EnableClock(kCLOCK_PortA);
	CLOCK_EnableClock(kCLOCK_PortB);
	CLOCK_EnableClock(kCLOCK_PortC);
	CLOCK_EnableClock(kCLOCK_PortD);
	CLOCK_EnableClock(kCLOCK_PortE);

	/* Osc pins */
	PORT_SetPinMux(PORTA, 18UL, kPORT_PinDisabledOrAnalog);
	PORT_SetPinMux(PORTA, 19UL, kPORT_PinDisabledOrAnalog);
#ifndef TESTER_BUILD

	can_tx_config.mux = kPORT_MuxAlt2;
	can_tx_config.openDrainEnable = kPORT_OpenDrainDisable;
	can_tx_config.pullSelect = kPORT_PullUp;
	can_tx_config.slewRate = kPORT_FastSlewRate;
	can_tx_config.passiveFilterEnable = kPORT_PassiveFilterDisable;
	can_tx_config.driveStrength = kPORT_LowDriveStrength;
	can_tx_config.lockRegister = kPORT_UnlockRegister;

	/* CAN0 pinmux config */
	PORT_SetPinConfig(PORTA, 12u, &can_tx_config); /* CAN0 TX */
	PORT_SetPinMux(PORTA, 13u, kPORT_MuxAlt2); /* CAN0 RX */

	/* CAN1 pinmux config */
	PORT_SetPinConfig(PORTC, 17u, &can_tx_config); /* CAN1 TX */
	PORT_SetPinMux(PORTC, 16u, kPORT_MuxAlt2); /* CAN1 RX */

#ifdef SDK_DEBUGCONSOLE
	/* Debug UART3 pinmux config */
	PORT_SetPinMux(PORTE, 0u, kPORT_MuxAlt3); /* UART1 TX */
	PORT_SetPinMux(PORTE, 1u, kPORT_MuxAlt3); /* UART1 RX */
#endif
#endif

#ifdef BOARD_USES_ADC
	/* Resistive Touch panel pinmux config */
	PORT_SetPinMux(PORTE, 6u, kPORT_MuxAsGpio);
	GPIO_PinInit(GPIOE, 6u, &gpio_out_hi_config); /* Force X+*/
	PORT_SetPinMux(PORTB, 9u, kPORT_MuxAsGpio);
	GPIO_PinInit(GPIOB, 9u, &gpio_out_config); /* Force X-*/
	PORT_SetPinMux(PORTC, 5u, kPORT_MuxAsGpio);
	GPIO_PinInit(GPIOC, 5u, &gpio_out_hi_config); /* Force Y+*/
	PORT_SetPinMux(PORTC, 13u, kPORT_MuxAsGpio);
	GPIO_PinInit(GPIOC, 13u, &gpio_out_config); /* Force Y-*/
	PORT_SetPinMux(PORTB, 6UL, kPORT_PinDisabledOrAnalog); /* Sense X+ */
	GPIO_PinInit(GPIOB, 6u, &gpio_in_config);
	PORT_SetPinMux(PORTB, 7UL, kPORT_PinDisabledOrAnalog); /* Sense X- */
	GPIO_PinInit(GPIOB, 7u, &gpio_in_config);
	PORT_SetPinMux(PORTC, 8UL, kPORT_PinDisabledOrAnalog); /* Sense Y+ */
	GPIO_PinInit(GPIOC, 8u, &gpio_in_config);
	PORT_SetPinMux(PORTC, 9UL, kPORT_PinDisabledOrAnalog); /* Sense Y- */
	GPIO_PinInit(GPIOC, 9u, &gpio_in_config);

	/* Apalis ADC pinmux config */
	PORT_SetPinMux(PORTB, 0UL, kPORT_PinDisabledOrAnalog); /* ADC0 */
	PORT_SetPinMux(PORTB, 1UL, kPORT_PinDisabledOrAnalog); /* ADC1 */
	PORT_SetPinMux(PORTB, 2UL, kPORT_PinDisabledOrAnalog); /* ADC2 */
	PORT_SetPinMux(PORTB, 3UL, kPORT_PinDisabledOrAnalog); /* ADC3 */
#endif
	/* SPI2 pinmux config */
	PORT_SetPinMux(PORTB, 21u, kPORT_MuxAlt2); /* SPI2_SCK */
	PORT_SetPinMux(PORTB, 22u, kPORT_MuxAlt2); /* SPI2_SOUT */
	PORT_SetPinMux(PORTB, 23u, kPORT_MuxAlt2); /* SPI2_SIN */
	PORT_SetPinMux(PORTB, 20u, kPORT_MuxAlt2); /* SPI2_SS */

	/* Open Drain INT pins config */
	od_config.mux = kPORT_MuxAsGpio;
	od_config.openDrainEnable = kPORT_OpenDrainEnable;
	od_config.pullSelect = kPORT_PullDisable;
	od_config.slewRate = kPORT_FastSlewRate;
	od_config.passiveFilterEnable = kPORT_PassiveFilterDisable;
	od_config.driveStrength = kPORT_LowDriveStrength;
	od_config.lockRegister = kPORT_UnlockRegister;
	GPIO_PinInit(GPIOA, 16u, &gpio_out_hi_config);
	PORT_SetPinConfig(PORTA, 16u, &od_config); /* MCU_INT1 */
	GPIO_PinInit(GPIOA, 29u, &gpio_out_hi_config);
	PORT_SetPinConfig(PORTA, 29u, &od_config); /* MCU_INT2 */
	GPIO_PinInit(GPIOB, 8u, &gpio_out_config);
	PORT_SetPinConfig(PORTB, 8u, &od_config); /* MCU_INT3 */
	GPIO_PinInit(GPIOE, 26u, &gpio_out_config);
	PORT_SetPinConfig(PORTE, 26u, &od_config); /* MCU_INT4 */
	GPIO_PinInit(GPIOC, 19u, &gpio_out_hi_config);
	PORT_SetPinConfig(PORTC, 19u, &od_config); /* PMIC_ONKEY */

	/* GPIOs */
	in_config.mux = kPORT_MuxAsGpio;
	in_config.openDrainEnable = kPORT_OpenDrainDisable;
	in_config.pullSelect = kPORT_PullDown;
	in_config.slewRate = kPORT_FastSlewRate;
	in_config.passiveFilterEnable = kPORT_PassiveFilterDisable;
	in_config.driveStrength = kPORT_LowDriveStrength;
	in_config.lockRegister = kPORT_UnlockRegister;
	for (i = 0; i < sizeof(gpio_list)/sizeof(struct gpio_id); i++){
		PORT_SetPinConfig(gpio_list[i].port, gpio_list[i].pin, &in_config);
		GPIO_PinInit(gpio_list[i].gpio, gpio_list[i].pin, &gpio_in_config);
	}

}
