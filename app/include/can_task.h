/*
 * can_task.h
 */

#ifndef SOURCE_CAN_TASK_H_
#define SOURCE_CAN_TASK_H_

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "fsl_dspi.h"

extern TaskHandle_t can0_task_handle;
extern TaskHandle_t can1_task_handle;
extern TaskHandle_t can_tx_notify_task_handle;
void can0_task(void *pvParameters);
void can1_task(void *pvParameters);
void can_tx_notify_task(void *pvParameters);
int canx_registers(dspi_transfer_t *spi_transfer, int id);
void can_spi_read_complete(uint8_t id);

#endif /* SOURCE_CAN_TASK_H_ */
