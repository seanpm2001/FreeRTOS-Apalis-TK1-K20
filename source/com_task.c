
#include "com_task.h"
#include "can_task.h"
#include "gpio_ext.h"
#include "adc_task.h"

volatile uint8_t registers[APALIS_TK1_K20_LAST_REG];
volatile uint8_t regRxHandled;

/* Put FW version at known address in a binary. Make it 32-bit to have room for the future*/
#ifndef TESTER_BUILD
const uint32_t __attribute__((section(".FwVersion"), used)) fw_version = APALIS_TK1_K20_FW_VER;
#else
const uint32_t __attribute__((section(".FwVersion"), used)) fw_version = APALIS_TK1_K20_TESTER_FW_VER;
#endif

static dspi_slave_handle_t spi_handle;
static uint8_t slaveRxData[APALIS_TK1_K20_MAX_BULK + APALIS_TK1_K20_HEADER];
static uint8_t slaveTxData[APALIS_TK1_K20_MAX_BULK + APALIS_TK1_K20_HEADER];

#define SPI_DMA

#ifdef SPI_DMA
dspi_slave_edma_handle_t g_dspi_edma_s_handle;
edma_handle_t dspiEdmaSlaveRxHandle;
edma_handle_t dspiEdmaSlaveTxHandle;
#endif
void generate_irq(uint8_t irq) {
	registers[APALIS_TK1_K20_IRQREG] |= BIT(irq);
	/* Toggle INT1 pin */
	GPIO_TogglePinsOutput(GPIOA, 1u << 16u);
}

void clear_irq_flag(uint8_t irq) {
	registers[APALIS_TK1_K20_IRQREG] &= ~irq;
}

uint8_t get_control_reg()
{
	return registers[APALIS_TK1_K20_CTRREG];

}

void set_control_reg(uint8_t value)
{
	registers[APALIS_TK1_K20_CTRREG] = value;
}

uint8_t get_status_reg()
{
	return registers[APALIS_TK1_K20_STAREG];

}

void set_status_reg(uint8_t value)
{
	registers[APALIS_TK1_K20_STAREG] = value;
}

uint8_t get_mask_reg()
{
	return registers[APALIS_TK1_K20_MSQREG];

}

void set_mask_reg(uint8_t value)
{
	registers[APALIS_TK1_K20_MSQREG] = value;
}

uint8_t get_irq_reg()
{
	return registers[APALIS_TK1_K20_IRQREG];

}

void set_irq_reg(uint8_t value)
{
	/* Clear IRQ flag on 1 */
	clear_irq_flag(value);

}

inline int general_registers(dspi_transfer_t * spi_transfer)
{
	uint8_t *rx_buf = spi_transfer->rxData;

	if (rx_buf[0] == APALIS_TK1_K20_WRITE_INST) {
		switch (rx_buf[1]) {
		case APALIS_TK1_K20_STAREG:
			set_status_reg(rx_buf[2]);
			return 1;
		case APALIS_TK1_K20_REVREG:
			return -ENOENT;
		case APALIS_TK1_K20_IRQREG:
			set_irq_reg(rx_buf[2]);
			return 1;
		case APALIS_TK1_K20_CTRREG:
			set_control_reg(rx_buf[2]);
			return 1;
		case APALIS_TK1_K20_MSQREG:
			set_mask_reg(rx_buf[2]);
			return 1;
		default:
			return -ESRCH;
		}
	}


	return -ENXIO;
}

static void SPI_callback(SPI_Type *base, dspi_slave_handle_t *handle, status_t status, void *userData)
{
	callback_message_t * cb = (callback_message_t*) userData;
	BaseType_t reschedule = pdFALSE;

	if (status == kStatus_Success)
	{
		xSemaphoreGiveFromISR(cb->sem, &reschedule);
	}

	if (status == kStatus_DSPI_Error)
	{
		__NOP();
	}
	portYIELD_FROM_ISR(reschedule);
}

static dspi_slave_config_t spi2_slaveConfig;

static void SPI_init() {
	gpio_pin_config_t gpio_out_config = {
			kGPIO_DigitalOutput, 0,
	};
	GPIO_PinInit(GPIOD, 11u, &gpio_out_config);
	/* Slave config */
	spi2_slaveConfig.whichCtar = kDSPI_Ctar0;
	spi2_slaveConfig.ctarConfig.bitsPerFrame = 8;
	spi2_slaveConfig.ctarConfig.cpol = kDSPI_ClockPolarityActiveHigh;
	spi2_slaveConfig.ctarConfig.cpha = kDSPI_ClockPhaseSecondEdge;
	spi2_slaveConfig.enableContinuousSCK = false;
	spi2_slaveConfig.enableRxFifoOverWrite = true;
	spi2_slaveConfig.enableModifiedTimingFormat = false;
	spi2_slaveConfig.samplePoint = kDSPI_SckToSin0Clock;
	PRINTF("SPI init \r\n");
	DSPI_SlaveInit(SPI2, &spi2_slaveConfig);
	DSPI_SlaveTransferCreateHandle(SPI2, &spi_handle, SPI_callback, spi_handle.userData);

	/* Set dspi slave interrupt priority higher. */
	NVIC_SetPriority(SPI2_IRQn, 5U);
	GPIO_ClearPinsOutput(GPIOA, 1u << 29u); /* INT2 active */
	PRINTF("SPI init done \r\n");

}

void spi_task(void *pvParameters) {
	callback_message_t spi_msg;
	dspi_transfer_t slaveXfer;
	int ret = 0;
	int can_read = -1;
#ifdef SPI_DMA
	uint32_t slaveRxChannel, slaveTxChannel;
	edma_config_t userConfig;
#endif

	spi_msg.sem = xSemaphoreCreateBinary();
	spi_handle.userData = &spi_msg;
	SPI_init();
#ifdef SPI_DMA
	slaveRxChannel = 0U;
	slaveTxChannel = 1U;

	DMAMUX_Init(DMAMUX);
	DMAMUX_SetSource(DMAMUX, slaveRxChannel, kDmaRequestMux0SPI2Rx);
	DMAMUX_EnableChannel(DMAMUX, slaveRxChannel);
	DMAMUX_SetSource(DMAMUX, slaveTxChannel, kDmaRequestMux0SPI2Tx);
	DMAMUX_EnableChannel(DMAMUX, slaveTxChannel);

	EDMA_GetDefaultConfig(&userConfig);
	EDMA_Init(DMA0, &userConfig);

	EDMA_CreateHandle(&dspiEdmaSlaveRxHandle, DMA0, slaveRxChannel);
	EDMA_CreateHandle(&dspiEdmaSlaveTxHandle, DMA0, slaveTxChannel);

	g_dspi_edma_s_handle.userData = &spi_msg;
	DSPI_SlaveTransferCreateHandleEDMA(SPI2, &g_dspi_edma_s_handle, SPI_callback, &spi_msg, &dspiEdmaSlaveRxHandle, &dspiEdmaSlaveTxHandle);
#endif
	memset(registers, 0x00, APALIS_TK1_K20_LAST_REG);
	registers[APALIS_TK1_K20_REVREG] = APALIS_TK1_K20_FW_VER;
	GPIO_SetPinsOutput(GPIOA, 1u << 29u); /* INT2 idle */
	slaveXfer.configFlags = kDSPI_SlaveCtar0;

	while(1){
		slaveXfer.txData = NULL;/* no MISO traffic*/
		slaveXfer.rxData = slaveRxData;
		slaveXfer.dataSize = 3;
		/* Wait for instructions from SoC */
		ret = DSPI_SlaveTransferNonBlocking(SPI2, &spi_handle, &slaveXfer);
		if ( ret == kStatus_Success) {

			xSemaphoreTake(spi_msg.sem, portMAX_DELAY);

			slaveXfer.txData = slaveTxData;
			slaveXfer.rxData = slaveRxData;
			if (slaveRxData[0] != APALIS_TK1_K20_READ_INST) {
				if (slaveRxData[1] <= 0x05) {
					ret = general_registers(&slaveXfer);
#ifndef TESTER_BUILD
				} else if ((slaveRxData[1] >= APALIS_TK1_K20_CANREG + APALIS_TK1_K20_CAN_DEV_OFFSET(0))
						&& (slaveRxData[1] <= APALIS_TK1_K20_CAN_OUT_BUF_END + APALIS_TK1_K20_CAN_DEV_OFFSET(0))) {
					ret = canx_registers(&slaveXfer, 0);
					can_read = 0;

				} else if ((slaveRxData[1] >= APALIS_TK1_K20_CANREG + APALIS_TK1_K20_CAN_DEV_OFFSET(1))
						&& (slaveRxData[1] <= APALIS_TK1_K20_CAN_OUT_BUF_END + APALIS_TK1_K20_CAN_DEV_OFFSET(1))) {
					ret = canx_registers(&slaveXfer, 1);
					can_read = 1;
#endif
#ifdef BOARD_USES_ADC
				} else if ((slaveRxData[1] >= APALIS_TK1_K20_ADCREG) && (slaveRxData[1] <= APALIS_TK1_K20_ADC_CH3H)) {
					ret = adc_registers(&slaveXfer);

				} else if ((slaveRxData[1] >= APALIS_TK1_K20_TSCREG) && (slaveRxData[1] <= APALIS_TK1_K20_TSC_YPH)) {
					ret = tsc_registers(&slaveXfer);
#endif
				} else if ((slaveRxData[1] >= APALIS_TK1_K20_GPIOREG) && (slaveRxData[1] <= APALIS_TK1_K20_GPIO_STA)) {
					ret = gpio_registers(&slaveXfer);
				} else {
					/* Register not defined */
					ret = -EINVAL;
				}


				if (ret < 0) {
					slaveTxData[0] = TK1_K20_INVAL;
					slaveTxData[1] = ret;
					slaveTxData[2] = slaveRxData[0];
					slaveTxData[3] = slaveRxData[1];
					slaveTxData[4] = slaveRxData[2];
					slaveXfer.txData = slaveTxData;
					ret = 5;
				}

				if (slaveRxData[0] == APALIS_TK1_K20_BULK_READ_INST)
				{
					slaveXfer.dataSize = ret;
					slaveXfer.rxData = NULL; /* We're not expecting any MOSI traffic, but NULL messes up stuff */
#ifdef SPI_DMA
					DSPI_SlaveTransferEDMA(SPI2, &g_dspi_edma_s_handle, &slaveXfer);
#else
					DSPI_SlaveTransferNonBlocking(SPI2, &spi_handle, &slaveXfer);
#endif
					xSemaphoreTake(spi_msg.sem, portMAX_DELAY);
					if (can_read >= 0) {
						can_spi_read_complete(can_read);
						can_read = -1;
					}
				}
			}

		}
	}
}
