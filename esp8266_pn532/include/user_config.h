/*
 * user_config.h
 *
 *      Author: Egzon Mulaj [PEBASYS LLC]
 */

#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__
#include "c_types.h"
#include "ets_sys.h"
#include "os_type.h"

////////Debug////////
/* Display data for debug if requested */
//#define DEBUG_WIFI
//#define PLATFORM_DEBUG
//#define PN532_CONNECTED_LEDS
//#define DEBUG_PN532

#define PN532_CONNECTED

#define SECOND_IN_MS 1000
#define SECOND_IN_US 1000000UL

os_timer_t pn532Timer;
os_timer_t initTimer;
os_timer_t enableInterruptTimer;

typedef enum {
	PN532_IDLE, PN532_FREE, PN532_BUSY
} pnStatus;

typedef enum {
	GPIO_SET_PULLUP = 3, GPIO_SET_NOPULL = 4
} EasyGPIO_PullStatus;

typedef enum {
	SHOWER_IDLE, SHOWER_ON_PAUZE, SHOWER_ON
} shState;

typedef enum PN532IRQ {
	IRQ_IDLE = 0,
	IRQ_ACK_FRAME,
	IRQ_FIRMWARE_VERSION,
	IRQ_SAMCONFIG,
	IRQ_PASSIVE_TARGET,
	IRQ_READ_UID,
	IRQ_READ_AUTHENTICATION,
	IRQ_READ_BLOCK_DATA,
	IRQ_WRITE_DATA,
	IRQ_PRE_SAM_CONFIG,
	IRQ_GPIO_WRITE_CMD,
	IRQ_GPIO_READ_RESP
} irq_int;

typedef enum PN532IRQSTATE {
	STATUS_IDLE = 0,
	GETTING_FIRMWARE,
	GETTING_SAMCONFIG,
	GETTING_CARD,
	GETTING_BLOCK_DATA,
	GETTING_BLOCK_AUTH,
	WRITING_BLOCK_DATA,
	WRITING_GPIO_DATA
} irq_state;

typedef enum {
	LED_COLOR_IDLE,
	LED_COLOR_NONE,
	LED_COLOR_RED,
	LED_COLOR_ORANGE,
	LED_COLOR_GREEN
} _LEDColor;

struct Wifi {
	uint8_t connection;
	char clientSsid[16];
	char clientPswd[16];
	char clientSsidTemp[16];
	char clientPswdTemp[16];
	char fwServerVersion[30];
	char serverIp[16];
	char serverPort[6];
	char serverIpTemp[16];
	char serverPortTemp[6];
	uint32 configurationData[70];
	uint8_t configurationChanges;
} wifi;

typedef struct {
	os_timer_t osLoopTimer;
	uint8_t _macAddr[6];
	char deviceMacAddr[13];
	bool deviceBooting;
	uint8_t aliveTimer;
} ESP8266;

ESP8266 esp8266;
shState showerState;
_LEDColor LEDColor;
irq_int pn532IrqInt;
irq_state pn532IrqState;

/////////////////////
uint8_t lockerStatus;
uint8_t indicatorLoadingArray[8];
static const uint8_t ULY[] = { 0U, 31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U,
		31U, 30U, 31U };

extern void ICACHE_FLASH_ATTR loop();

#endif

