/*
 * com_task.h
 */

#ifndef COM_TASK_H_
#define COM_TASK_H_

#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_dspi.h"
#include "fsl_gpio.h"
#include "fsl_edma.h"
#include "fsl_dspi_edma.h"
#include "fsl_dmamux.h"
#include "apalis-tk1-k20-api.h"

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "errno.h"

#define SPI_DMA

typedef struct _callback_message_t
{
	status_t async_status;
	SemaphoreHandle_t sem;
} callback_message_t;

extern TaskHandle_t spi_task_handle;
void generate_irq(uint8_t irq);
void clear_irq_flag(uint8_t irq);
void spi_task(void *pvParameters);

#define BIT(nr)                 (1UL << (nr))
#define ADC0_CHANNEL_CNT	4
#define TSC0_CHANNEL_CNT	4
#define CAN_RX_BUF_SIZE		256

extern volatile uint8_t  registers[APALIS_TK1_K20_LAST_REG];

#endif /* COM_TASK_H_ */
