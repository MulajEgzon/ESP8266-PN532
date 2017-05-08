#include "ets_sys.h"
#include "user_interface.h"
#include "user_config.h"
#include "os_type.h"
#include "osapi.h"
#include "gpio.h"
#include "pn532.h"
#include "gpio_pins.h"
#include "mem.h"

void ICACHE_FLASH_ATTR pn532SetIrqPin(int pin) {

	switch (pin) {
	case 4:
		ets_uart_printf("This I2C_SDA pin, Please change IRQ_PIN !\n");
//		while (true)
//			;
		return;
	case 5:
		ets_uart_printf("This I2C_SCL pin, Please change IRQ_PIN !\n");
//		while (true)
//			;
		return;
	case 0:
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
		break;
	case 2:
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
		break;
	case 3:
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_GPIO3);
		break;
	case 9:
		PIN_FUNC_SELECT(FUNC_SPIHD, FUNC_GPIO9);
		break;
	case 10:
		PIN_FUNC_SELECT(FUNC_SPIWP, FUNC_GPIO10);
		break;
	case 12:
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
		break;
	case 13:
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
		break;
	case 14:
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
		break;
	case 15:
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15);
		break;
	default:
		ets_uart_printf("\nNOT AN INTERRUPT PIN !\n");
		return;

	}
	//gpio_output_set(0, 0, 0, GPIO_ID_PIN(IRQ_PIN));
	ETS_GPIO_INTR_ATTACH(&pn532IntrHandler, 0);
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(pin));
	gpio_pin_intr_state_set(GPIO_ID_PIN(pin), GPIO_PIN_INTR_ANYEDGE);
	ETS_GPIO_INTR_ENABLE();
}

// interrupt handler
void ICACHE_FLASH_ATTR pn532IntrHandler(uint32 interruptMask, void *arg) {
	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
	gpio_pin_intr_state_set(GPIO_ID_PIN(IRQ_PIN), GPIO_PIN_INTR_DISABLE);
	if (gpio_status & BIT(IRQ_PIN)) {
//		ets_uart_printf("pn532IrqInt:%u\n",pn532IrqInt);
//		ets_uart_printf("pn532IrqState:%u\n",pn532IrqState);
		//COMMAND
		switch (pn532IrqInt) {
		case IRQ_GPIO_READ_RESP:
			pn532_readGpioResp();
			break;
		case IRQ_IDLE:
			break;
		case IRQ_PRE_SAM_CONFIG:
//			pn532ReadPreSAMConfig();
			break;
		case IRQ_ACK_FRAME:
			pn532MifareReadAckFrame();
			break;
		case IRQ_FIRMWARE_VERSION:
			//_getFirmwareVersion();
			break;
		case IRQ_READ_UID:
			pn532ReadCardUid();
			break;
		case IRQ_SAMCONFIG:
//			pn532MifareReadSAMConfig();
			break;
		case IRQ_READ_AUTHENTICATION:
			pn532ReadAuthenticateResponse();
			break;
		case IRQ_READ_BLOCK_DATA:
			pn532MifareReadDataBlock();
			break;
		case IRQ_WRITE_DATA:
			pn532ReadWritenDataFromBlock();
			break;
		default:
//			ets_uart_printf("default: pn532IrqInt ->%u\n",pn532IrqInt);
//			ets_uart_printf("default: pn532IrqState ->%u\n",pn532IrqState);
			break;
		}
		GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(IRQ_PIN));
		//gpio_pin_intr_state_set(GPIO_ID_PIN(0), GPIO_PIN_INTR_ANYEDGE);
	}
	gpio_pin_intr_state_set(GPIO_ID_PIN(IRQ_PIN), GPIO_PIN_INTR_ANYEDGE);
}

