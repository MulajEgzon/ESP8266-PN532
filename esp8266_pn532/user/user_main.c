#include "ets_sys.h"
#include "driver/uart.h"

#include "gpio.h"
#include "gpio_pins.h"
#include "os_type.h"
#include "c_types.h"
#include "user_interface.h"
#include "user_config.h"
#include "driver/brzo_i2c.h"
#include "spi_flash.h"
#include "pn532.h"
#include "wifi.h"
#include "upgrade.h"
#include "osapi.h"
#include "time.h"

void ICACHE_FLASH_ATTR bootingInfo()
{
#ifdef PLATFORM_DEBUG
	struct rst_info * reset_info;
	ets_uart_printf("\nmac_address : ");
	printlnArrayHex(esp8266._macAddr, 6);
	ets_uart_printf("cpu_freq : %u\n", system_get_cpu_freq());
	ets_uart_printf("spi flash size : %u\n", system_get_flash_size_map());
	ets_uart_printf("SDK version %s\n", system_get_sdk_version());
	ets_uart_printf("system_get_userbin_addr : %u\n",
			system_get_userbin_addr());
	ets_uart_printf("system_upgrade_flag_check() : %u\n",
			system_upgrade_flag_check());
//#ifdef PLATFORM_DEBUG
	user_rf_cal_sector_set();
	OUT_OF_RANGE(258);
	ets_uart_printf("----free heap: %u----\n", system_get_free_heap_size());
//	ets_uart_printf("system_get_boot_version(); %s\n", system_get_boot_version());
	reset_info = system_get_rst_info();
	ets_uart_printf("reset_info->reason %u\n", reset_info->reason);
	ets_uart_printf("reset_info->exccause %u\n", reset_info->exccause);
	ets_uart_printf("reset_info->excvaddr %u\n", reset_info->excvaddr);
	ets_uart_printf("reset_info->epc1 %u\n", reset_info->epc1);
	ets_uart_printf("reset_info->epc2 %u\n", reset_info->epc2);
	ets_uart_printf("reset_info->epc3 %u\n", reset_info->epc3);
	ets_uart_printf("reset_info->depc %u\n", reset_info->depc);
#endif
}

uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void) {
	enum flash_size_map size_map = system_get_flash_size_map();
#ifdef FLASH_DEBUG
	ets_uart_printf("size_map :%u\n", size_map);
#endif
	uint32 rf_cal_sec = 0;

	switch (size_map) {
	case FLASH_SIZE_4M_MAP_256_256:
		rf_cal_sec = 128 - 8;
		break;

	case FLASH_SIZE_8M_MAP_512_512:
		rf_cal_sec = 256 - 5;
		break;

	case FLASH_SIZE_16M_MAP_512_512:
	case FLASH_SIZE_16M_MAP_1024_1024:
		rf_cal_sec = 512 - 5;
		break;

	case FLASH_SIZE_32M_MAP_512_512:
	case FLASH_SIZE_32M_MAP_1024_1024:
		rf_cal_sec = 1024 - 5;
		break;

	default:
		rf_cal_sec = 0;
		break;
	}
	return rf_cal_sec;
}

void post_user_init_func() {
//	os_printf("\n\n\npost_user_init_func() called\r\n");
	wifi_configuration();
	os_timer_disarm(&initTimer);
#ifdef PN532_CONNECTED
		os_timer_setfn(&initTimer, (os_timer_func_t *) pn532Connection, NULL);
#else
	os_timer_setfn(&initTimer, (os_timer_func_t *) loop, NULL);
#endif
	os_timer_arm(&initTimer, 1000,0 );
}

void ICACHE_FLASH_ATTR user_init() {

	uart_init(BIT_RATE_74880, BIT_RATE_74880);
	bootingInfo();
	gpio_init();
	ETS_GPIO_INTR_DISABLE();
	gpio_pin_intr_state_set(GPIO_ID_PIN(IRQ_PIN), GPIO_PIN_INTR_DISABLE);
	brzo_i2c_setup(I2C_MASTER_SDA_GPIO, I2C_MASTER_SCL_GPIO, I2C_STRETCHING_CLOCK);
	initVariables();
	system_init_done_cb(&post_user_init_func);


}

