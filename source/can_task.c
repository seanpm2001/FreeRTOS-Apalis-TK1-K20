
#include "can_task.h"
#include "com_task.h"
#include "fsl_flexcan.h"

/* special address description flags for the CAN_ID */
#define CAN_EFF_FLAG 0x80000000U /* EFF/SFF is set in the MSB */
#define CAN_RTR_FLAG 0x40000000U /* remote transmission request */
#define CAN_ERR_FLAG 0x20000000U /* error message frame */

#define TX_MESSAGE_BUFFER_NUM0 (9)

#define CAN_FRAME_MAX_LEN	8
#define CAN_HEADER_MAX_LEN	5
#define CAN_TRANSFER_BUF_LEN	(CAN_HEADER_MAX_LEN + CAN_FRAME_MAX_LEN)

#define CANCTRL_MODMASK		(BIT(1) | BIT(0))
#define CANCTRL_INTEN		BIT(2)
#define CANINTF_RX		BIT(3)
#define CANINTF_TX		BIT(4)
#define CANINTF_ERR		BIT(5)
#define CANCTRL_ENABLE		BIT(6)
#define CANCTRL_INTMASK	(CANINTF_RX | CANINTF_TX | CANINTF_ERR)

#define EFLG_EWARN	0x01
#define EFLG_RXWAR	0x02
#define EFLG_TXWAR	0x04
#define EFLG_RXEP	0x08
#define EFLG_TXEP	0x10
#define EFLG_TXBO	0x20
#define EFLG_RXOVR	0x40

struct can_registers {
	CAN_Type *base;
	flexcan_handle_t handle;
	uint8_t can_enable;
	uint8_t can_err_reg;
	uint8_t can_mode;
	uint8_t frames_in_buf;
	uint8_t rx_buf_top;
	uint8_t rx_buf_bottom;
	uint8_t tx_counter;
};

static uint8_t data_buffer[2][CAN_RX_BUF_SIZE][CAN_TRANSFER_BUF_LEN];

static struct can_registers can_regs[2];

static inline void generate_can_irq(uint8_t id)
{
	if (id == 0) {
		GPIO_TogglePinsOutput(GPIOB, 1u << 8u);
	} else {
		GPIO_TogglePinsOutput(GPIOE, 1u << 26u);
	}
}

void can_tx_notify_task(void *pvParameters)
{
	uint32_t ulInterruptStatus;

	while(1){
		xTaskNotifyWait( 0x00, 0xFFFFFFFF, &ulInterruptStatus, portMAX_DELAY);
		if (ulInterruptStatus & 0x01) {
			registers[APALIS_TK1_K20_CANREG + APALIS_TK1_K20_CAN_DEV_OFFSET(0)] &= ~CANINTF_TX;
			generate_can_irq(0);
		}
		if (ulInterruptStatus & 0x02) {
			registers[APALIS_TK1_K20_CANREG + APALIS_TK1_K20_CAN_DEV_OFFSET(1)] &= ~CANINTF_TX;
			generate_can_irq(1);
		}
		if (ulInterruptStatus & 0x04) {
			registers[APALIS_TK1_K20_CANREG + APALIS_TK1_K20_CAN_DEV_OFFSET(0)] |= CANINTF_ERR;
			generate_can_irq(0);
		}
		if (ulInterruptStatus & 0x08) {
			registers[APALIS_TK1_K20_CANREG + APALIS_TK1_K20_CAN_DEV_OFFSET(1)] |= CANINTF_ERR;
			generate_can_irq(1);
		}
	}
}

static BaseType_t flexcan_callback(CAN_Type *base, flexcan_handle_t *handle, status_t status, uint32_t result, void *userData)
{
	callback_message_t * cb = (callback_message_t*) userData;
	BaseType_t reschedule = pdFALSE;

	switch (status)
	{
	case kStatus_FLEXCAN_RxFifoIdle:
		cb->async_status = pdTRUE;
		xSemaphoreGiveFromISR(cb->sem, &reschedule);
		break;

	case kStatus_FLEXCAN_TxIdle:
		if (TX_MESSAGE_BUFFER_NUM0 == result)
		{
			switch ((int)base)
			{
			case (int)CAN0:

				xTaskNotifyFromISR(can_tx_notify_task_handle, 0x01, eSetBits, &reschedule);
				break;
			case (int)CAN1:
				xTaskNotifyFromISR(can_tx_notify_task_handle, 0x02, eSetBits, &reschedule);
				break;
			}

		}
		break;

	case kStatus_FLEXCAN_ErrorStatus:
		if ((result & (kFLEXCAN_TxWarningIntFlag)) != 0) {
			switch ((int)base)
			{
			case (int)CAN0:
				registers[APALIS_TK1_K20_CANERR + APALIS_TK1_K20_CAN_DEV_OFFSET(0)] |= EFLG_TXWAR;
				xTaskNotifyFromISR(can_tx_notify_task_handle, 0x04, eSetBits, &reschedule);
				break;
			case (int)CAN1:
				registers[APALIS_TK1_K20_CANERR + APALIS_TK1_K20_CAN_DEV_OFFSET(1)] |= EFLG_TXWAR;
				xTaskNotifyFromISR(can_tx_notify_task_handle, 0x08, eSetBits, &reschedule);
				break;
			}
		}

		if ((result & (kFLEXCAN_BusOffIntFlag)) != 0) {
			switch ((int)base)
			{
			case (int)CAN0:
				registers[APALIS_TK1_K20_CANERR + APALIS_TK1_K20_CAN_DEV_OFFSET(0)] |= EFLG_TXBO;
				xTaskNotifyFromISR(can_tx_notify_task_handle, 0x04, eSetBits, &reschedule);
				break;
			case (int)CAN1:
				registers[APALIS_TK1_K20_CANERR + APALIS_TK1_K20_CAN_DEV_OFFSET(1)] |= EFLG_TXBO;
				xTaskNotifyFromISR(can_tx_notify_task_handle, 0x08, eSetBits, &reschedule);
				break;
			}
		}
		break;
	default:
		break;
	}

	return reschedule;
}

static void CAN_Init(uint8_t id)
{
	flexcan_config_t flexcanConfig;
	flexcan_rx_fifo_config_t fifoConfig;
	uint32_t fifoFilter = 0xFFFFFFFF;

	FLEXCAN_GetDefaultConfig(&flexcanConfig);

	flexcanConfig.baudRate = 1000000U; /* set default to 1Mbit/s*/

	/* Init FlexCAN module. */
	flexcanConfig.clkSrc = kFLEXCAN_ClkSrcPeri;
	FLEXCAN_Init(can_regs[id].base, &flexcanConfig, CLOCK_GetFreq(kCLOCK_BusClk));

	/* Create FlexCAN handle structure and set call back function. */
	FLEXCAN_TransferCreateHandle(can_regs[id].base, &can_regs[id].handle, flexcan_callback,
			can_regs[id].handle.userData);

	/* Set Rx Mask to don't care on all bits. */
	FLEXCAN_SetRxMbGlobalMask(can_regs[id].base, FLEXCAN_RX_MB_EXT_MASK(0x00, 0, 0));
	FLEXCAN_SetRxFifoGlobalMask(can_regs[id].base, FLEXCAN_RX_MB_EXT_MASK(0x00, 0, 0));

	fifoConfig.idFilterNum = 0;
	fifoConfig.idFilterTable = &fifoFilter;
	fifoConfig.idFilterType = kFLEXCAN_RxFifoFilterTypeC;
	fifoConfig.priority = kFLEXCAN_RxFifoPrioHigh;
	FLEXCAN_SetRxFifoConfig(can_regs[id].base, &fifoConfig, true);

	/* errata #5641 */
	can_regs[id].base->MB[TX_MESSAGE_BUFFER_NUM0 - 1].ID = 0x0;
	can_regs[id].base->MB[TX_MESSAGE_BUFFER_NUM0 - 1].WORD0 = 0x0;
	can_regs[id].base->MB[TX_MESSAGE_BUFFER_NUM0 - 1].WORD1 = 0x0;
	can_regs[id].base->MB[TX_MESSAGE_BUFFER_NUM0 - 1].CS = CAN_CS_CODE(0x8);
	can_regs[id].base->MB[TX_MESSAGE_BUFFER_NUM0 - 1].CS = CAN_CS_CODE(0x8);

	/* Setup Tx Message Buffer. */
	FLEXCAN_SetTxMbConfig(can_regs[id].base, TX_MESSAGE_BUFFER_NUM0, true);
}

uint8_t available_data[2];

static void can_calculate_available_data(uint8_t id) {
	taskENTER_CRITICAL();
	if ( can_regs[id].rx_buf_bottom <= can_regs[id].rx_buf_top)
		available_data[id] = can_regs[id].rx_buf_top - can_regs[id].rx_buf_bottom;
	else
		available_data[id] = CAN_RX_BUF_SIZE - can_regs[id].rx_buf_bottom;

	available_data[id] = (available_data[id] > APALIS_TK1_MAX_CAN_DMA_XREF) ? APALIS_TK1_MAX_CAN_DMA_XREF:available_data[id];
	registers[APALIS_TK1_K20_CAN_IN_BUF_CNT + APALIS_TK1_K20_CAN_DEV_OFFSET(id)] = available_data[id];

	if (can_regs[id].rx_buf_bottom == can_regs[id].rx_buf_top)
		registers[APALIS_TK1_K20_CANREG + APALIS_TK1_K20_CAN_DEV_OFFSET(id)] &= ~CANINTF_RX;
	else
		registers[APALIS_TK1_K20_CANREG + APALIS_TK1_K20_CAN_DEV_OFFSET(id)] |= CANINTF_RX;
	taskEXIT_CRITICAL();
}

void can_spi_read_complete(uint8_t id)
{
	can_calculate_available_data(id);
}

static void frame_to_buffer(flexcan_frame_t *frame, uint8_t id) {
	uint8_t top_frame = can_regs[id].rx_buf_top;
	switch (frame->format){
	case kFLEXCAN_FrameFormatExtend:
		data_buffer[id][top_frame][0] = frame->id & 0xFF;
		data_buffer[id][top_frame][1] = (frame->id >> 8 ) & 0xFF;
		data_buffer[id][top_frame][2] = (frame->id >> 16 ) & 0xFF;
		data_buffer[id][top_frame][3] = (frame->id >> 24 ) & 0x1F;
		data_buffer[id][top_frame][3] |= CAN_EFF_FLAG >> 24;
		break;
	case kFLEXCAN_FrameFormatStandard:
		data_buffer[id][top_frame][0] = (frame->id >> 18) & 0xFF;
		data_buffer[id][top_frame][1] = ((frame->id >> 18) >> 8 ) & 0x7F;
		data_buffer[id][top_frame][2] = 0x00;
		data_buffer[id][top_frame][3] = 0x00;
		break;
	}

	data_buffer[id][top_frame][3] |= frame->type ?  (CAN_RTR_FLAG >> 24):0x00;
	data_buffer[id][top_frame][4] = frame->length;

	switch (frame->length) {
	case 8:data_buffer[id][top_frame][5 + 7] = frame->dataByte7;
	case 7:data_buffer[id][top_frame][5 + 6] = frame->dataByte6;
	case 6:data_buffer[id][top_frame][5 + 5] = frame->dataByte5;
	case 5:data_buffer[id][top_frame][5 + 4] = frame->dataByte4;
	case 4:data_buffer[id][top_frame][5 + 3] = frame->dataByte3;
	case 3:data_buffer[id][top_frame][5 + 2] = frame->dataByte2;
	case 2:data_buffer[id][top_frame][5 + 1] = frame->dataByte1;
	case 1:data_buffer[id][top_frame][5] = frame->dataByte0;
	}

	can_regs[id].rx_buf_top++;
	can_regs[id].rx_buf_top %= CAN_RX_BUF_SIZE;

	can_calculate_available_data(id);
}

static inline void can_fifo_rx(uint8_t id, flexcan_fifo_transfer_t * rxXfer)
{
	callback_message_t *can_msg = (callback_message_t *) can_regs[id].handle.userData;

	FLEXCAN_TransferReceiveFifoNonBlocking(can_regs[id].base, &can_regs[id].handle, rxXfer);
	xSemaphoreTake(can_msg->sem, portMAX_DELAY);
	if (can_msg->async_status == pdTRUE) {
		frame_to_buffer(rxXfer->frame, id);
		can_regs[id].frames_in_buf++;
		generate_can_irq(id);
		if (can_regs[id].frames_in_buf >= CAN_RX_BUF_SIZE)
			vTaskSuspend(NULL);
	}
}

void can0_task(void *pvParameters) {
	flexcan_frame_t  rxFrame;
	flexcan_fifo_transfer_t rxXfer;
	callback_message_t can_msg;

	can_msg.sem = xSemaphoreCreateBinary();
	can_msg.async_status = pdFALSE;
	memset(&can_regs[0], 0x00u, sizeof(struct can_registers));
	can_regs[0].handle.userData = (void *) &can_msg;
	can_regs[0].base = CAN0;

	CAN_Init(0);
	PRINTF("CAN0 init done \r\n");

	rxXfer.frame = &rxFrame;

	while(1)
	{
		can_fifo_rx(0, &rxXfer);
	}
	vSemaphoreDelete(can_msg.sem);
}

void can1_task(void *pvParameters) {
	flexcan_frame_t  rxFrame;
	flexcan_fifo_transfer_t rxXfer;
	callback_message_t can_msg;

	can_msg.sem = xSemaphoreCreateBinary();
	can_msg.async_status = pdFALSE;
	memset(&can_regs[1], 0x00u, sizeof(struct can_registers));
	can_regs[1].handle.userData = (void *) &can_msg;
	can_regs[1].base = CAN1;

	CAN_Init(1);
	PRINTF("CAN1 init done \r\n");
	rxXfer.frame = &rxFrame;

	while(1)
	{
		can_fifo_rx(1, &rxXfer);
	}
	vSemaphoreDelete(can_msg.sem);

}

static void can_change_mode(int id, uint8_t new_mode)
{
	can_regs[id].can_mode = new_mode;

	FLEXCAN_SetMode(can_regs[id].base, new_mode);

}

static void can_enable(int id, uint8_t enable)
{
	can_regs[id].can_mode = enable;

	if (enable) {
		callback_message_t *can_msg = (callback_message_t *) can_regs[id].handle.userData;
		FLEXCAN_ExitFreezeMode(can_regs[id].base);
		can_msg->async_status = pdFALSE;
		xSemaphoreGive(can_msg->sem);
		generate_can_irq(id);
	} else {
		FLEXCAN_TransferAbortReceiveFifo(can_regs[id].base, &can_regs[id].handle);
		FLEXCAN_EnterFreezeMode(can_regs[id].base);
	}

}

static uint8_t set_canreg (int id, uint8_t value)
{
	registers[APALIS_TK1_K20_CANREG + APALIS_TK1_K20_CAN_DEV_OFFSET(id)] = value;
	if ( can_regs[id].can_mode != (value & CANCTRL_MODMASK) )
		can_change_mode(id, (value & CANCTRL_MODMASK));
	if (can_regs[id].can_enable != (value & CANCTRL_ENABLE))
		can_enable(id, (value & CANCTRL_ENABLE));
	return 0;
}

static uint8_t clr_canreg (int id, uint8_t mask)
{
	mask &= (CANINTF_RX | CANINTF_TX | CANINTF_ERR);
	registers[APALIS_TK1_K20_CANREG + APALIS_TK1_K20_CAN_DEV_OFFSET(id)] &= ~mask;
	return 0;
}

static uint8_t set_canerr (int id, uint8_t value)
{
	registers[APALIS_TK1_K20_CANERR + APALIS_TK1_K20_CAN_DEV_OFFSET(id)] = value;
	return 0;
}

static uint8_t set_canbadreg (int id, uint8_t value)
{

	FLEXCAN_SetBitRate(can_regs[id].base, CLOCK_GetFreq(kCLOCK_BusClk), value * APALIS_TK1_CAN_CLK_UNIT);
	return 0;
}

static uint8_t set_canbittiming (int id, uint16_t value, int16_t mask)
{
	/* According to NXP we should use default settings */
	return 0;
}

uint8_t can_sendframe(uint8_t id, uint8_t *data, uint8_t len)
{
	flexcan_mb_transfer_t txXfer;
	flexcan_frame_t tx_frame;

	tx_frame.length = data[4];
	tx_frame.id = (data[3] << 24) + (data[2] << 16) + (data[1] << 8) + data[0];

	tx_frame.format = (data[3] << 24 & CAN_EFF_FLAG) ?
				kFLEXCAN_FrameFormatExtend:kFLEXCAN_FrameFormatStandard;
	tx_frame.type = (data[3] << 24 & CAN_RTR_FLAG) ?
				kFLEXCAN_FrameTypeRemote:kFLEXCAN_FrameTypeData;

	if (tx_frame.format == kFLEXCAN_FrameFormatExtend)
		tx_frame.id = FLEXCAN_ID_EXT(tx_frame.id);
	else
		tx_frame.id = FLEXCAN_ID_STD(tx_frame.id);

	tx_frame.dataByte0 = data[5];
	tx_frame.dataByte1 = data[5 + 1];
	tx_frame.dataByte2 = data[5 + 2];
	tx_frame.dataByte3 = data[5 + 3];
	tx_frame.dataByte4 = data[5 + 4];
	tx_frame.dataByte5 = data[5 + 5];
	tx_frame.dataByte6 = data[5 + 6];
	tx_frame.dataByte7 = data[5 + 7];

	txXfer.frame = &tx_frame;
	txXfer.mbIdx = TX_MESSAGE_BUFFER_NUM0;
	if( tx_frame.length <= 8 ) {
		FLEXCAN_TransferSendNonBlocking(can_regs[id].base , &can_regs[id].handle, &txXfer);
		registers[APALIS_TK1_K20_CANREG + APALIS_TK1_K20_CAN_DEV_OFFSET(id)] |= CANINTF_TX;
		return 0;
	}
	else {
		registers[APALIS_TK1_K20_CANREG + APALIS_TK1_K20_CAN_DEV_OFFSET(id)] &= ~CANINTF_TX;
		generate_can_irq(id);
		return -EIO;
	}
}

uint16_t can_readframe(uint8_t id, dspi_transfer_t *spi_transfer)
{
	uint8_t rx_size;
	spi_transfer->txData = data_buffer[id][can_regs[id].rx_buf_bottom];
	rx_size = spi_transfer->rxData[2] / CAN_TRANSFER_BUF_LEN; /* size in frames, not bytes */
	if (rx_size > available_data[id])
		rx_size = available_data[id];

	can_regs[id].rx_buf_bottom = can_regs[id].rx_buf_bottom + rx_size;
	can_regs[id].rx_buf_bottom %= CAN_RX_BUF_SIZE;

	can_regs[id].frames_in_buf -= rx_size;

	return rx_size * CAN_TRANSFER_BUF_LEN;
}

int canx_registers(dspi_transfer_t *spi_transfer, int id)
{
	uint8_t *rx_buf = spi_transfer->rxData;
	if (rx_buf[0] == APALIS_TK1_K20_WRITE_INST) {
		rx_buf[1] -= APALIS_TK1_K20_CAN_DEV_OFFSET(id);
		switch (rx_buf[1]) {
		case APALIS_TK1_K20_CANREG:
			return set_canreg(id, rx_buf[2]);
		case APALIS_TK1_K20_CANREG_CLR:
			return clr_canreg(id, rx_buf[2]);
		case APALIS_TK1_K20_CANERR:
			return set_canerr(id, rx_buf[2]);
		case APALIS_TK1_K20_CAN_BAUD_REG:
			return set_canbadreg(id, rx_buf[2]);
		case APALIS_TK1_K20_CAN_BIT_1:
			return set_canbittiming(id, rx_buf[2], 0x00FF);
		case APALIS_TK1_K20_CAN_BIT_2:
			return set_canbittiming(id, (rx_buf[2] << 8), 0xFF00);
		default:
			return -EIO;
		}

	} else if (rx_buf[0] == APALIS_TK1_K20_BULK_READ_INST) {
		rx_buf[1] -= APALIS_TK1_K20_CAN_DEV_OFFSET(id);
		switch (rx_buf[1]) {
		case APALIS_TK1_K20_CAN_IN_BUF:
			return can_readframe(id, spi_transfer);
		default:
			return -EIO;
		}
	} else if (rx_buf[0] == APALIS_TK1_K20_BULK_WRITE_INST) {
		rx_buf[1] -= APALIS_TK1_K20_CAN_DEV_OFFSET(id);
		switch (rx_buf[1]) {
		case APALIS_TK1_K20_CAN_OUT_BUF:
			can_sendframe(id, &rx_buf[3], rx_buf[2]);
			return 0;
		default:
			return -EIO;
		}
	}
	return -EIO;
}

