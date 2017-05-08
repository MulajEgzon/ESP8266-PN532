/*
 * WiFi.h
 *
 *  Created on: Jul 21, 2016
 *      Author: PC
 */

#ifndef  WIFI_H_INCLUDED
#define  WIFI_H_INCLUDED

#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"
#include "driver/uart.h"
#include "espconn.h"


#define USE_WIFI_MODE			STATIONAP_MODE

typedef enum {
	WIFI_CONNECTING,
	WIFI_CONNECTING_ERROR,
	WIFI_CONNECTED,
} tConnState;

void 	ICACHE_FLASH_ATTR wifi_configuration();
void 	ICACHE_FLASH_ATTR wifi_check_ip(void *arg);
void 	ICACHE_FLASH_ATTR wifiEventHandler(System_Event_t *event);
void 	ICACHE_FLASH_ATTR list_lwip_tcp_psc(void);
void 	ICACHE_FLASH_ATTR setup_wifi_ap_mode(void);
void 	ICACHE_FLASH_ATTR setup_wifi_st_mode(void);
void 	ICACHE_FLASH_ATTR request_scan(void);
void 	ICACHE_FLASH_ATTR init_wifi_scan_cb(void);
void 	ICACHE_FLASH_ATTR tcp_recv_cb(void *arg, char *pdata, unsigned short len);


#endif

