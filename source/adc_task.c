/*
 * adc_task.c
 *
 */

#include "com_task.h"
#include "adc_task.h"
#include "fsl_adc16.h"
#include "fsl_port.h"
#include "errno.h"
#include "stdlib.h"

struct adc_data {
	uint16_t adc[ADC0_CHANNEL_CNT];
	uint16_t tsc_xm;
	uint16_t tsc_xp;
	uint16_t tsc_ym;
	uint16_t tsc_yp;
} adc_data;

/*
 * Apalis ADC0 -> PTB0 -> ADC0_SE8
 * Apalis ADC1 -> PTB1 -> ADC0_SE9
 * Apalis ADC2 -> PTB2 -> ADC0_SE12
 * Apalis ADC3 -> PTB3 -> ADC0_SE13
 *
 * Touch screen:
 * 	Force:
 * 			PTE6	X+ hi-off
 * 			PTB9	X- lo-off
 * 			PTC5	Y+ hi-off
 * 			PTC13	Y- lo-off
 * 	Sense:
 * 		PTB7 -> ADC1_SE13	X-
 * 		PTB6 -> ADC1_SE12	X+
 * 		PTC9 -> ADC1_SE5b	Y-
 * 		PTC8 -> ADC1_SE4b	Y+
 */
const uint8_t adc0_channels[ADC0_CHANNEL_CNT] = {8, 9, 12, 13};
const uint8_t tsc_channels[TSC0_CHANNEL_CNT] = {13, 12, 5, 4};

static int adc_task_init(void)
{
	adc16_config_t adc_config;
#ifdef BOARD_USES_ADC
	ADC16_GetDefaultConfig(&adc_config);
	adc_config.resolution = kADC16_ResolutionSE16Bit;
	adc_config.longSampleMode = kADC16_LongSampleDisabled;
	adc_config.clockDivider = kADC16_ClockDivider8;
	ADC16_Init(ADC0, &adc_config);
	ADC16_SetHardwareAverage(ADC0, kADC16_HardwareAverageDisabled);
	PRINTF("ADC init done \r\n");
	return 0;
#else
	return -ENODEV;
#endif
}

static int tsc_task_init(void)
{
	adc16_config_t adc_config;
#ifdef BOARD_USES_ADC
	ADC16_GetDefaultConfig(&adc_config);
	adc_config.resolution = kADC16_ResolutionSE12Bit;
	adc_config.longSampleMode = kADC16_LongSampleCycle10;
	adc_config.clockDivider = kADC16_ClockDivider8;
	ADC16_Init(ADC1, &adc_config);
	ADC16_SetChannelMuxMode(ADC1, kADC16_ChannelMuxB);
	ADC16_SetHardwareAverage(ADC1, kADC16_HardwareAverageCount32);
	PRINTF("TSC init done \r\n");
	return 0;
#else
	return -ENODEV;
#endif
}

#ifdef BOARD_USES_ADC
static void ts_force_drive(uint8_t xm, uint8_t xp, uint8_t ym, uint8_t yp)
{
	if (xp == 0)
		GPIO_SetPinsOutput(GPIOE, 1 << 6);
	else
		GPIO_ClearPinsOutput(GPIOE, 1 << 6);

	if (xm > 0)
		GPIO_SetPinsOutput(GPIOB, 1 << 9);
	else
		GPIO_ClearPinsOutput(GPIOB, 1 << 9);

	if (yp == 0)
		GPIO_SetPinsOutput(GPIOC, 1 << 5);
	else
		GPIO_ClearPinsOutput(GPIOC, 1 << 5);

	if (ym > 0)
		GPIO_SetPinsOutput(GPIOC, 1 << 13);
	else
		GPIO_ClearPinsOutput(GPIOC, 1 << 13);
}

static inline uint16_t do_adc_conversion(ADC_Type *base, adc16_channel_config_t *channel)
{
	ADC16_SetChannelConfig(base, 0, channel);
	while(ADC16_GetChannelStatusFlags(base, 0) == 0){vTaskDelay(0);}
	return (uint16_t)ADC16_GetChannelConversionValue(base, 0);
}

void adc_task(void *pvParameters)
{
	adc16_channel_config_t channel;
	int i;
	if (adc_task_init() < 0)
		return;

	channel.enableDifferentialConversion = false;
	channel.enableInterruptOnConversionCompleted = false;

	while(1) {
		for (i = 0; i < ADC0_CHANNEL_CNT;i ++){
			channel.channelNumber = adc0_channels[i];
			adc_data.adc[i] = do_adc_conversion(ADC0, &channel);
			registers[APALIS_TK1_K20_ADC_CH0L + 2 * i] = adc_data.adc[i] & 0xFF;
			registers[APALIS_TK1_K20_ADC_CH0L + 2 * i + 1] = (adc_data.adc[i] >> 8) & 0xFF;
		}
		vTaskDelay(5);
	}

}

enum touch_status {
	PEN_UP,
	PEN_DOWN
};

#define MIN_TOUCH_DET	5

/* 		PTB7 -> ADC1_SE13	X-
 * 		PTB6 -> ADC1_SE12	X+
 * 		PTC9 -> ADC1_SE5b	Y-
 * 		PTC8 -> ADC1_SE4b	Y+
 */
void tsc_task(void *pvParameters)
{
	adc16_channel_config_t channel;
	port_pin_config_t pin_config_pd, pin_config_ana;
	int old_status, status = PEN_UP;
	int irq_stat = 0, press;
	uint16_t tsc_xm;
	uint16_t tsc_xp;
	uint16_t tsc_ym;
	uint16_t tsc_yp;

	if (tsc_task_init() < 0)
		return;

	pin_config_pd.mux = kPORT_MuxAsGpio;
	pin_config_pd.openDrainEnable = kPORT_OpenDrainDisable;
	pin_config_pd.pullSelect = kPORT_PullUp;
	pin_config_pd.slewRate = kPORT_FastSlewRate;
	pin_config_pd.passiveFilterEnable = kPORT_PassiveFilterDisable;
	pin_config_pd.driveStrength = kPORT_LowDriveStrength;
	pin_config_pd.lockRegister = kPORT_UnlockRegister;

	pin_config_ana.mux = kPORT_PinDisabledOrAnalog;
	pin_config_ana.openDrainEnable = kPORT_OpenDrainDisable;
	pin_config_ana.pullSelect = kPORT_PullDisable;
	pin_config_ana.slewRate = kPORT_FastSlewRate;
	pin_config_ana.passiveFilterEnable = kPORT_PassiveFilterDisable;
	pin_config_ana.driveStrength = kPORT_LowDriveStrength;
	pin_config_ana.lockRegister = kPORT_UnlockRegister;

	channel.enableDifferentialConversion = false;
	channel.enableInterruptOnConversionCompleted = false;

	while(1) {
		//Touch detect: power Y-, enable pullup on xp and read xp GPIO
		ts_force_drive(0, 0, 1, 0);
		PORT_SetPinConfig(PORTB, 6u, &pin_config_pd);
		vTaskDelay(5);
		old_status = status;
		status = GPIO_ReadPinInput(GPIOB, 6u) ? PEN_UP:PEN_DOWN;
		PORT_SetPinConfig(PORTB, 6u, &pin_config_ana);

		if (status != old_status)
			irq_stat = 0;

		if (status == PEN_DOWN) {
			//probe ym with power across Y plane
			ts_force_drive(0, 0, 1, 1);
			channel.channelNumber = tsc_channels[0];
			tsc_ym = do_adc_conversion(ADC1, &channel);

			//probe yp with power across Y plane
			channel.channelNumber = tsc_channels[1];
			tsc_yp = do_adc_conversion(ADC1, &channel);

			//probe xm with power across X plane
			ts_force_drive(1, 1, 0, 0);
			channel.channelNumber = tsc_channels[2];
			tsc_xm = do_adc_conversion(ADC1, &channel);

			//probe xp with power across X plane
			channel.channelNumber = tsc_channels[3];
			tsc_xp = do_adc_conversion(ADC1, &channel);

#ifdef TOUCH_STRONG_FILTERING
			/* additional filtering */
			//probe ym with power across Y plane
			ts_force_drive(0, 0, 1, 1);
			channel.channelNumber = tsc_channels[0];
			tsc_ym += do_adc_conversion(ADC1, &channel);
			tsc_ym >>= 1;

			//probe yp with power across Y plane
			channel.channelNumber = tsc_channels[1];
			tsc_yp += do_adc_conversion(ADC1, &channel);
			tsc_yp >>= 1;

			//probe xm with power across X plane
			ts_force_drive(1, 1, 0, 0);
			channel.channelNumber = tsc_channels[2];
			tsc_xm += do_adc_conversion(ADC1, &channel);
			tsc_xm >>= 1;

			//probe xp with power across X plane
			channel.channelNumber = tsc_channels[3];
			tsc_xp += do_adc_conversion(ADC1, &channel);
			tsc_xp >>= 1;
#endif

			/* make sure that pen is making good contact */
			press = (abs(tsc_yp - tsc_ym)
					+ abs(tsc_xp - tsc_xm)) / 2;
#ifdef TOUCH_STRONG_FILTERING
			if (press > 9) {
#else
			if (press > 15) {
#endif
				continue;
			} else {
				adc_data.tsc_xm = tsc_xm;
				adc_data.tsc_xp = tsc_xp;
				adc_data.tsc_ym = tsc_ym;
				adc_data.tsc_yp = tsc_yp;
			}

		} else {
			adc_data.tsc_xm = 0;
			adc_data.tsc_xp = 0;
			adc_data.tsc_ym = 0;
			adc_data.tsc_yp = 0;
			vTaskDelay(10);
		}

		if (irq_stat == 0) {
			generate_irq(APALIS_TK1_K20_TSC_IRQ);
			irq_stat = 1;
		}

		vTaskDelay(5);
	}

}

int tsc_registers(dspi_transfer_t *spi_transfer)
{
	uint8_t *rx_buf = spi_transfer->rxData;
	uint8_t *tx_buf = &spi_transfer->txData[0];

	if (rx_buf[0] == APALIS_TK1_K20_WRITE_INST) {
		switch (rx_buf[1]) {
		case APALIS_TK1_K20_TSCREG:
			return -ENOENT;
		default:
			return -ENOENT;
		}
	} else if (rx_buf[0] == APALIS_TK1_K20_BULK_READ_INST) {
		if (rx_buf[1] == APALIS_TK1_K20_TSC_XML) {
			if (rx_buf[2] == 2) {
				tx_buf[0] = adc_data.tsc_xm & 0xFF;
				tx_buf[1] = (adc_data.tsc_xm >> 8) & 0xFF;
				return 2;
			} else if (rx_buf[2] == 8) {
				tx_buf[0] = adc_data.tsc_xm & 0xFF;
				tx_buf[1] = (adc_data.tsc_xm >> 8) & 0xFF;
				tx_buf[2] = adc_data.tsc_xp & 0xFF;
				tx_buf[3] = (adc_data.tsc_xp >> 8) & 0xFF;
				tx_buf[4] = adc_data.tsc_ym & 0xFF;
				tx_buf[5] = (adc_data.tsc_ym >> 8) & 0xFF;
				tx_buf[6] = adc_data.tsc_yp & 0xFF;
				tx_buf[7] = (adc_data.tsc_yp >> 8) & 0xFF;
				return 8;
			}
		}
		switch (rx_buf[1]) {
		case APALIS_TK1_K20_TSC_XPL:
			tx_buf[0] = adc_data.tsc_xp & 0xFF;
			tx_buf[1] = (adc_data.tsc_xp >> 8) & 0xFF;
			return 2;
		case APALIS_TK1_K20_TSC_YML:
			tx_buf[0] = adc_data.tsc_ym & 0xFF;
			tx_buf[1] = (adc_data.tsc_ym >> 8) & 0xFF;
			return 2;
		case APALIS_TK1_K20_TSC_YPL:
			tx_buf[0] = adc_data.tsc_yp & 0xFF;
			tx_buf[1] = (adc_data.tsc_yp >> 8) & 0xFF;
			return 2;
		default:
			return -ENOENT;
		}
	}
	return -ENOENT;
}

int adc_registers(dspi_transfer_t *spi_transfer)
{
	uint8_t *rx_buf = spi_transfer->rxData;
	uint8_t *tx_buf = &spi_transfer->txData[0];

	if (rx_buf[0] == APALIS_TK1_K20_WRITE_INST) {
		switch (rx_buf[1]) {
		case APALIS_TK1_K20_ADCREG:
			return -ENOENT;
		default:
			return -ENOENT;
		}
	} else if (rx_buf[0] == APALIS_TK1_K20_BULK_READ_INST) {
		if (rx_buf[1] == APALIS_TK1_K20_ADC_CH0L) {
			if (rx_buf[2] == 2) {
				tx_buf[0] = adc_data.adc[0] & 0xFF;
				tx_buf[1] = (adc_data.adc[0] >> 8) & 0xFF;
				return 2;
			} else if (rx_buf[2] == 8) {
				tx_buf[0] = adc_data.adc[0] & 0xFF;
				tx_buf[1] = (adc_data.adc[0] >> 8) & 0xFF;
				tx_buf[2] = adc_data.adc[1] & 0xFF;
				tx_buf[3] = (adc_data.adc[1] >> 8) & 0xFF;
				tx_buf[4] = adc_data.adc[2] & 0xFF;
				tx_buf[5] = (adc_data.adc[2] >> 8) & 0xFF;
				tx_buf[6] = adc_data.adc[3] & 0xFF;
				tx_buf[7] = (adc_data.adc[3] >> 8) & 0xFF;
				return 8;
			}
		}
		switch (rx_buf[1]){
		case APALIS_TK1_K20_ADC_CH1L:
			tx_buf[0] = adc_data.adc[1] & 0xFF;
			tx_buf[1] = (adc_data.adc[1] >> 8) & 0xFF;
			return 2;
		case APALIS_TK1_K20_ADC_CH2L:
			tx_buf[0] = adc_data.adc[2] & 0xFF;
			tx_buf[1] = (adc_data.adc[2] >> 8) & 0xFF;
			return 2;
		case APALIS_TK1_K20_ADC_CH3L:
			tx_buf[0] = adc_data.adc[3] & 0xFF;
			tx_buf[1] = (adc_data.adc[3] >> 8) & 0xFF;
			return 2;

		default:
			return -ENOENT;
		}
	}
	return -ENOENT;
}
#endif
