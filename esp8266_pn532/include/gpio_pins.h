#include "ets_sys.h"
#include "user_config.h"
#include "gpio.h"

#define IRQ_PIN				14

#ifndef I2C_MASTER_SDA_GPIO
#define I2C_MASTER_SDA_GPIO 4
#endif

#ifndef I2C_MASTER_SCL_GPIO
#define I2C_MASTER_SCL_GPIO 5
#endif

#ifndef I2C_STRETCHING_CLOCK
#define I2C_STRETCHING_CLOCK 2089
#endif

void ICACHE_FLASH_ATTR pn532SetIrqPin(int pin);

void ICACHE_FLASH_ATTR pn532IntrHandler(uint32 interruptMask, void *arg);
