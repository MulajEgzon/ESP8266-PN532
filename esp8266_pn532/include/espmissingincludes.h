/*
 * This file is part of Espruino, a JavaScript interpreter for Microcontrollers
 *
 * Copyright (C) 2015 Gordon Williams <gw@pur3.co.uk>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ----------------------------------------------------------------------------
 * This file is designed to be parsed during the build process
 *
 * Contains ESP8266 board specific functions.
 * ----------------------------------------------------------------------------
 */
#ifndef ESPMISSINGINCLUDES_H
#define ESPMISSINGINCLUDES_H

#include <user_interface.h>
#include <eagle_soc.h>

// The linking is arranged to put all functions into IROM, so we need a special define to put
// a function into IRAM
#define CALLED_FROM_INTERRUPT __attribute__((section(".iram1.text")))

//Missing function prototypes in include folders. Gcc will warn on these if we don't define 'em anywhere.
//MOST OF THESE ARE GUESSED! but they seem to work and shut up the compiler.

bool 	ICACHE_FLASH_ATTR wifi_station_set_hostname(char *);
char *	ICACHE_FLASH_ATTR wifi_station_get_hostname(void);

int 	ICACHE_FLASH_ATTR atoi(const char *nptr);

void 	ICACHE_FLASH_ATTR ets_install_putc1(void *routine); // necessary for #define os_xxx -> ets_xxx
void 	ICACHE_FLASH_ATTR ets_isr_attach(int intr, void *handler, void *arg);
void 	ICACHE_FLASH_ATTR ets_isr_mask(unsigned intr);
void 	ICACHE_FLASH_ATTR ets_isr_unmask(unsigned intr);
void 	ICACHE_FLASH_ATTR ets_intr_lock(void);
void 	ICACHE_FLASH_ATTR ets_intr_unlock(void);
void 	ICACHE_FLASH_ATTR NmiTimSetFunc(void (*func)(void));

int 	ICACHE_FLASH_ATTR 	ets_memcmp(const void *s1, const void *s2, size_t n);
void *	ICACHE_FLASH_ATTR ets_memcpy(void *dest, const void *src, size_t n);
void *	ICACHE_FLASH_ATTR ets_memmove(void *dest, const void *src, size_t n);
void *	ICACHE_FLASH_ATTR ets_memset(void *s, int c, size_t n);
int 	ICACHE_FLASH_ATTR ets_sprintf(char *str, const char *format, ...)  __attribute__ ((format (printf, 2, 3)));
int 	ICACHE_FLASH_ATTR ets_str2macaddr(void *, void *);
int 	ICACHE_FLASH_ATTR ets_strcmp(const char *s1, const char *s2);
char *	ICACHE_FLASH_ATTR ets_strcpy(char *dest, const char *src);
size_t 	ICACHE_FLASH_ATTR ets_strlen(const char *s);
int 	ICACHE_FLASH_ATTR ets_strncmp(const char *s1, const char *s2, int len);
char *	ICACHE_FLASH_ATTR ets_strncpy(char *dest, const char *src, size_t n);
char *	ICACHE_FLASH_ATTR ets_strstr(const char *haystack, const char *needle);

void 	ICACHE_FLASH_ATTR ets_timer_arm_new(ETSTimer *a, int b, int c, int isMstimer);
void 	ICACHE_FLASH_ATTR ets_timer_disarm(ETSTimer *a);
void 	ICACHE_FLASH_ATTR ets_timer_setfn(ETSTimer *t, ETSTimerFunc *fn, void *parg);

void 	ICACHE_FLASH_ATTR ets_update_cpu_frequency(int freqmhz);

int 	ICACHE_FLASH_ATTR os_snprintf(char *str, size_t size, const char *format, ...) __attribute__((format(printf, 3, 4)));
int 	ICACHE_FLASH_ATTR os_printf_plus(const char *format, ...)  __attribute__((format(printf, 1, 2)));

void 	ICACHE_FLASH_ATTR  vPortFree(void *ptr, char * file, int line);
void *	ICACHE_FLASH_ATTR pvPortMalloc(size_t xWantedSize, char * file, int line);
void *	ICACHE_FLASH_ATTR pvPortZalloc(size_t, char * file, int line);
void *	ICACHE_FLASH_ATTR vPortMalloc(size_t xWantedSize);
void  	ICACHE_FLASH_ATTR pvPortFree(void *ptr);
void *	ICACHE_FLASH_ATTR pvPortRealloc(void *pv, size_t size, char * file, int line);

void 	ICACHE_FLASH_ATTR uart_div_modify(int no, unsigned int freq);
uint32 	ICACHE_FLASH_ATTR system_get_time();
int 	ICACHE_FLASH_ATTR rand(void);
void 	ICACHE_FLASH_ATTR ets_bzero(void *s, size_t n);
void 	ICACHE_FLASH_ATTR ets_delay_us(int ms);

#define os_timer_done ets_timer_done
#define os_timer_handler_isr ets_timer_handler_isr
#define os_timer_init ets_timer_init

#undef PIN_FUNC_SELECT
#define PIN_FUNC_SELECT(PIN_NAME, FUNC)  do { \
    WRITE_PERI_REG(PIN_NAME,   \
        (READ_PERI_REG(PIN_NAME) & ~(PERIPHS_IO_MUX_FUNC<<PERIPHS_IO_MUX_FUNC_S))  \
            |( (((FUNC&BIT2)<<2)|(FUNC&0x3))<<PERIPHS_IO_MUX_FUNC_S) );  \
    } while (0)
#endif
