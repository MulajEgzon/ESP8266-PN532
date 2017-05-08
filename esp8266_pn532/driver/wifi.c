/*
 * WiFi.c
 *
 *  Created on: Jul 21, 2016
 *      Author: PC
 */

#include "wifi.h"
#include "os_type.h"
#include "ets_sys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "gpio.h"
#include "pn532.h"
#include "osapi.h"

void ICACHE_FLASH_ATTR wifi_configuration() {
#ifdef PLATFORM_DEBUG
	ets_uart_printf("ESP8266 platform starting...\r\n");
#endif
	//{
#ifdef PLATFORM_DEBUG
	ets_uart_printf("ESP8266 is %s mode, restarting in %s mode...\r\n",
			WiFiMode[wifi_get_opmode()], WiFiMode[USE_WIFI_MODE]);
#endif

	setup_wifi_st_mode();
//	if (USE_WIFI_MODE & SOFTAP_MODE) {
//		setup_wifi_ap_mode();
//	if (USE_WIFI_MODE & STATION_MODE)
//		setup_wifi_st_mode();
////	}
	if (USE_WIFI_MODE & SOFTAP_MODE)
		wifi_get_macaddr(SOFTAP_IF, esp8266._macAddr);
	if (USE_WIFI_MODE & STATION_MODE)
		wifi_get_macaddr(STATION_IF, esp8266._macAddr);
	if (wifi_get_phy_mode() != PHY_MODE_11N)
		wifi_set_phy_mode(PHY_MODE_11N);
	if (wifi_station_get_auto_connect() == 0)
		wifi_station_set_auto_connect(1);

#ifdef PLATFORM_DEBUG
	ets_uart_printf("Wi-Fi mode: %s\r\n", WiFiMode[wifi_get_opmode()]);
	if (USE_WIFI_MODE & SOFTAP_MODE) {
		struct softap_config apConfig;
		if (wifi_softap_get_config(&apConfig)) {
			ets_uart_printf("AP config: SSID: %s, PASSWORD: %s\r\n",
					apConfig.ssid, apConfig.password);
		}
	}
	if (USE_WIFI_MODE & STATION_MODE) {
		struct station_config stationConfig;
		if (wifi_station_get_config(&stationConfig)) {
			ets_uart_printf("STA config: SSID: %s, PASSWORD: %s\r\n",
					stationConfig.ssid, stationConfig.password);
		}
	}
#endif
#ifdef PLATFORM_DEBUG
	ets_uart_printf("ESP8266 platform started!\r\n");
#endif

//	wifi.connection = false;
//	pn532.sendReadUidCmd = true;
	wifi_set_event_handler_cb(wifiEventHandler);
//	os_timer_setfn(&timer, (os_timer_func_t *) loop, NULL);
//	os_timer_arm(&timer, 1000, 0);
}

void ICACHE_FLASH_ATTR wifiEventHandler(System_Event_t *event) {
	switch (event->event) {
#ifdef	DEBUG_WIFI
	case EVENT_STAMODE_CONNECTED:

	ets_uart_printf("EVENT_STAMODE_CONNECTED\n");
	ets_uart_printf("connect to ssid %s, channel %d\n",
			event->event_info.connected.ssid,
			event->event_info.connected.channel);
	wifi.connection = false;

	break;
#endif
	case EVENT_STAMODE_DISCONNECTED:
#ifdef	DEBUG_WIFI
		ets_uart_printf("Event: EVENT_STAMODE_DISCONNECTED\n");
		ets_uart_printf("disconnect from ssid %s, reason %d\n",
				event->event_info.disconnected.ssid,
				event->event_info.disconnected.reason);
#endif
		wifi.connection = false;
		break;
#ifdef	DEBUG_WIFI
		case EVENT_STAMODE_AUTHMODE_CHANGE:
		ets_uart_printf("Event: EVENT_STAMODE_AUTHMODE_CHANGE\n");
		ets_uart_printf("mode: %d -> %d\n",
				event->event_info.auth_change.old_mode,
				event->event_info.auth_change.new_mode);
		break;
#endif
	case EVENT_STAMODE_GOT_IP:
		wifi.connection = true;
//#ifdef	DEBUG_WIFI

		ets_uart_printf("Event: EVENT_STAMODE_GOT_IP\n");
		ets_uart_printf("ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR,
				IP2STR(&event->event_info.got_ip.ip),
				IP2STR(&event->event_info.got_ip.mask),
				IP2STR(&event->event_info.got_ip.gw));
//#endif

		break;
#ifdef	DEBUG_WIFI
		case EVENT_SOFTAPMODE_STACONNECTED:
		ets_uart_printf("Event: EVENT_SOFTAPMODE_STACONNECTED\n");
		ets_uart_printf("station: " MACSTR "join, AID = %d\n",
				MAC2STR(event->event_info.sta_connected.mac),
				event->event_info.sta_connected.aid);
		break;
		case EVENT_SOFTAPMODE_STADISCONNECTED:
		ets_uart_printf("station: " MACSTR "leave, AID = %d\n",
				MAC2STR(event->event_info.sta_disconnected.mac),
				event->event_info.sta_disconnected.aid);
		break;
		case EVENT_SOFTAPMODE_PROBEREQRECVED:
		ets_uart_printf(
				"Event: EVENT_SOFTAPMODE_PROBEREQRECVED\nJust INGORE IT ! \n");
		break;
		case EVENT_MAX:
		ets_uart_printf("Event: EVENT_MAX\nJust INGORE IT ! \n");
		break;
#endif
	default:
#ifdef	DEBUG_WIFI
		ets_uart_printf("Unexpected event: %d\n", event->event);
#endif
		break;
	}
}

void ICACHE_FLASH_ATTR setup_wifi_st_mode(void) {
	wifi_set_opmode_current(STATION_MODE);
	struct station_config stconfig;
	wifi_station_disconnect();
	wifi_station_dhcpc_stop();
	if (wifi_station_get_config(&stconfig)) {
		os_memset(stconfig.ssid, 0, sizeof(stconfig.ssid));
		os_memset(stconfig.password, 0, sizeof(stconfig.password));
#ifdef PLATFORM_DEBUG
		ets_uart_printf("\nSSID : %s\n ", wifi.clientSsid);
		ets_uart_printf("\nPSWD : %s\n", wifi.clientPswd);
#endif
		os_sprintf(stconfig.ssid, "%s", wifi.clientSsid);
		os_sprintf(stconfig.password, "%s", wifi.clientPswd);
		if (!wifi_station_set_config(&stconfig)) {
#ifdef PLATFORM_DEBUG
			ets_uart_printf("ESP8266 not set station config!\r\n");
#endif
		} else {
#ifdef PLATFORM_DEBUG
			ets_uart_printf("ESP8266 Set station config!\r\n");
#endif
		}
	}
	wifi_station_connect();
	wifi_station_dhcpc_start();
	wifi_station_set_auto_connect(1);

#ifdef PLATFORM_DEBUG
	ets_uart_printf("ESP8266 in STA mode configured.\r\n");
#endif
}

void ICACHE_FLASH_ATTR loop() {
	ets_uart_printf("\nSSID :%s, PSW : %s", wifi.clientSsid, wifi.clientPswd);
	if (wifi.connection) {
		ets_uart_printf("  ||  FREE HEAP: %u\n", system_get_free_heap_size());

#ifdef PLATFORM_DEBUG
		ets_uart_printf("\nWIFI CONNECTED");
#endif
	} else {
#ifdef PLATFORM_DEBUG

		ets_uart_printf("\nTrying to connect with ssid : %s, heap %u", wifi.clientSsid, system_get_free_heap_size());
#endif
		flushArray(pn532.tempUid, pn532.tempUidLen);
#ifdef PN532_CONNECTED_LEDS
		pn532_led_color(LED_COLOR_NONE);
		os_delay_us(100000);
		pn532_led_color(LED_COLOR_ORANGE);
		os_delay_us(100000);
		pn532_led_color(LED_COLOR_NONE);
#endif
	}
//	ets_uart_printf("\nCALLING LOOP IN ...%u ms", SECOND_IN_MS*5);
	os_timer_disarm(&esp8266.osLoopTimer);
	os_timer_setfn(&esp8266.osLoopTimer, (os_timer_func_t *) loop, NULL);
	os_timer_arm(&esp8266.osLoopTimer, SECOND_IN_MS*5, 0);

}
