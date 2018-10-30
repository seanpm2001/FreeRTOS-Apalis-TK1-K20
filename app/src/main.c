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

/**
 * This is template for main module created by New Kinetis SDK 2.x Project Wizard. Enjoy!
 **/

#include <string.h>

#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "com_task.h"
#include "can_task.h"
#include "adc_task.h"

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"


#if ((defined USB_HOST_CONFIG_KHCI) && (USB_HOST_CONFIG_KHCI))
#define CONTROLLER_ID kUSB_ControllerKhci0
#endif
#if ((defined USB_HOST_CONFIG_EHCI) && (USB_HOST_CONFIG_EHCI))
#define CONTROLLER_ID kUSB_ControllerEhci0
#endif

#ifdef BOARD_USES_ADC
TaskHandle_t adc_task_handle;
TaskHandle_t tsc_task_handle;
#endif
#ifndef TESTER_BUILD
TaskHandle_t can0_task_handle;
TaskHandle_t can1_task_handle;
TaskHandle_t can_tx_notify_task_handle;
#endif
TaskHandle_t spi_task_handle;


/*!
 * @brief Application entry point.
 */
int main(void) {

	/* Init board hardware. */
	BOARD_InitPins();
	BOARD_BootClockRUN();
#ifndef TESTER_BUILD
	BOARD_InitDebugConsole();
#endif
	PRINTF("Apalis K20 Firmware Version %d.%d\r\n", FW_MAJOR, FW_MINOR);

	/* Create RTOS task */
	if(xTaskCreate(spi_task, "SPI_task", 1000L / sizeof(portSTACK_TYPE), NULL, 4, &spi_task_handle) != pdPASS)
	{
		PRINTF("create SPI task error\r\n");
	}
#ifndef TESTER_BUILD
	if(xTaskCreate(can0_task, "CAN0_task", 1000L / sizeof(portSTACK_TYPE), NULL, 2, &can0_task_handle) != pdPASS)
	{
		PRINTF("create CAN0 task error\r\n");
	}

	if(xTaskCreate(can1_task, "CAN1_task", 1000L / sizeof(portSTACK_TYPE), NULL, 2, &can1_task_handle) != pdPASS)
	{
		PRINTF("create CAN1 task error\r\n");
	}

	if(xTaskCreate(can_tx_notify_task, "CAN_tx_notify_task", 1000L / sizeof(portSTACK_TYPE), NULL, 3, &can_tx_notify_task_handle) != pdPASS)
	{
		PRINTF("create CAN TX notify task error\r\n");
	}
#endif
#ifdef BOARD_USES_ADC
	if(xTaskCreate(adc_task, "ADC_task", 1000L / sizeof(portSTACK_TYPE), NULL, 2, &adc_task_handle) != pdPASS)
	{
		PRINTF("create ADC task error\r\n");
	}

	if(xTaskCreate(tsc_task, "TSC_task", 1000L / sizeof(portSTACK_TYPE), NULL, 2, &tsc_task_handle) != pdPASS)
	{
		PRINTF("create TSC task error\r\n");
	}
#endif

	NVIC_SetPriority(CAN0_ORed_Message_buffer_IRQn, 7u);
	NVIC_SetPriority(CAN0_Bus_Off_IRQn, 8u);
	NVIC_SetPriority(CAN0_Error_IRQn, 8u);
	NVIC_SetPriority(CAN0_Tx_Warning_IRQn, 8u);
	NVIC_SetPriority(CAN0_Rx_Warning_IRQn, 8u);
	NVIC_SetPriority(CAN0_Wake_Up_IRQn, 8u);
	NVIC_SetPriority(CAN1_ORed_Message_buffer_IRQn, 7u);
	NVIC_SetPriority(CAN1_Bus_Off_IRQn, 8u);
	NVIC_SetPriority(CAN1_Error_IRQn, 8u);
	NVIC_SetPriority(CAN1_Tx_Warning_IRQn, 8u);
	NVIC_SetPriority(CAN1_Rx_Warning_IRQn, 8u);
	NVIC_SetPriority(CAN1_Wake_Up_IRQn, 8u);
	NVIC_SetPriority(SPI2_IRQn, 5u);
	NVIC_SetPriority(DMA0_IRQn, 6u);
	vTaskStartScheduler();

	for(;;) { /* Infinite loop to avoid leaving the main function */
		__asm("NOP"); /* something to use as a breakpoint stop while looping */
	}
}



