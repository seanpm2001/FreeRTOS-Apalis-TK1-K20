/*
 * adc_task.h
 *
 */

#ifndef SOURCE_ADC_TASK_H_
#define SOURCE_ADC_TASK_H_
#include "board.h"
#include "fsl_dspi.h"

#ifdef BOARD_USES_ADC
extern TaskHandle_t adc_task_handle;
extern TaskHandle_t tsc_task_handle;
void adc_task(void *pvParameters);
void tsc_task(void *pvParameters);
int adc_registers(dspi_transfer_t *spi_transfer);
int tsc_registers(dspi_transfer_t *spi_transfer);
#endif

#endif /* SOURCE_ADC_TASK_H_ */
