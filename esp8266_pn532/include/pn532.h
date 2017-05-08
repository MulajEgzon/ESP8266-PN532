/*
 * PN532ReadWrite.h
 *
 *  Created on: Jun 28, 2016
 *      Author: Egzon Mulaj [PEBASYS LLC]
 */

#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"
#include "user_config.h"
#include "driver/brzo_i2c.h"

#define pn532_brzo_start_transaction()		brzo_i2c_start_transaction(0x24,400)
#define PN532_I2C_ADDRESS                   (0x24)

// PN532 Commands
#define PN532_PREAMBLE                      (0x00)
#define PN532_STARTCODE2                    (0xFF)
#define PN532_POSTAMBLE                     (0x00)
#define PN532_HOSTTOPN532                   (0xD4)
#define PN532_PN532TOHOST                   (0xD5)
#define PN532_COMMAND_GETFIRMWAREVERSION    (0x02)
#define PN532_COMMAND_SAMCONFIGURATION      (0x14)
#define PN532_COMMAND_INLISTPASSIVETARGET   (0x4A)
#define PN532_COMMAND_INDATAEXCHANGE        (0x40)
#define PN532_RESPONSE_INDATAEXCHANGE       (0x41)
#define PN532_RESPONSE_INLISTPASSIVETARGET  (0x4B)
#define PN532_MIFARE_ISO14443A              (0x00)

// Mifare Commands
#define MIFARE_CMD_AUTH_A                   (0x60)
#define MIFARE_CMD_AUTH_B                   (0x61)
#define MIFARE_CMD_READ                     (0x30)
#define MIFARE_CMD_WRITE                    (0xA0)

//GPIO CMD
#define PN532_COMMAND_WRITEGPIO             (0x0E)
#define PN532_GPIO_VALIDATIONBIT            (0x80)
#define PN532_GPIO_P30                      (0)
#define PN532_GPIO_P31                      (1)
#define PN532_GPIO_P32                      (2)
#define PN532_GPIO_P33                      (3)
#define PN532_GPIO_P34                      (4)
#define PN532_GPIO_P35                      (5)

#define PN532_PACKBUFFSIZ 						64
#define PN532_TIMEOUT_OVER_SECOND				1
#define PN532_ENABLE_INTERRUPT_TIMEOUT			2000
#define PN532_ENABLE_INTERRUPT_TIMEOUT_5000		5000
#define PN532_READ_SECTOR_2						7
#define PN532_AUTHENTICATE_WRITING_SECTOR		3
#define PN532_I2C_BUFFER_LENGTH					0x1C
#define PN532_I2C_CMD_LENGTH					0x1C
#define PN532_ACK_BUFFER_LENGTH					0x06
#define PN532_WRITING_BLOCK						1
#define PN532_READING_SSID_BLOCK				(1)
#define PN532_READING_PSWD_BLOCK				(3)
#define PN532_READ_LAST_BLOCK					(5)
#define PN532_MAX_UID_LENGTH					0x7
#define SECTOR_0								3

#define CARD_MAX_LENGTH							7
#define MIFARE_KEY_B							1
#define MIFARE_KEY_A							0

//Writes data in block 1
//#define PN532_WRITING_DATA

//Reads data from block 1 and block 3
//#define PN532_READING_BLOCK


typedef struct {
	bool booting;	//= true;
//	bool sendReadUidCmd;
	bool masterCard;
	bool userCard;
	uint8_t _irq;
	uint8_t tempUidLen;  // uid len
	uint8_t uidLength;
	uint8_t authenticated;	// = 0;
	uint8_t authenticateBlock;
	uint8_t readingBlock;
	uint8_t cmdLength;
	uint8_t * cmd;
	uint8_t dataInBlock[16];
	uint8_t keyUniversal[6];	// = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	uint8_t recvBuffer[PN532_I2C_BUFFER_LENGTH];
	uint8_t uid[7];	//= { 0, 0, 0, 0, 0, 0 };
	uint8_t tempUid[7];	//= { 0, 0, 0, 0, 0, 0 };
} pn532Struct;
static pn532Struct pn532;

// static funcions that can be called only in pn532.c file

static void ICACHE_FLASH_ATTR pn532SendAuthenticationKey(uint8_t block);

static bool ICACHE_FLASH_ATTR _pn532SendAuthenticationKey(uint8_t * uid,
		uint8_t uidLen, uint8_t blockNumber, uint8_t keyNumber,
		uint8_t * keyData);

static bool ICACHE_FLASH_ATTR pn532SamConfig();

static uint32_t ICACHE_FLASH_ATTR pn532GetFirmwareVersion(void);

static void ICACHE_FLASH_ATTR pn532ReadPassiveTargetID(uint8_t cardbaudrate,
		uint8_t * uid, uint8_t * uidLength);

static bool ICACHE_FLASH_ATTR pn532ReadyToReadPassiveTargetID(
		uint8_t cardbaudrate, uint8_t * uid, uint8_t * uidLength);

static bool ICACHE_FLASH_ATTR pn532ReadAckFrame(char * _array, uint8_t _length);

static bool ICACHE_FLASH_ATTR pn532MifareSendReadBlockCommand(
		uint8_t blockNumber);

static bool ICACHE_FLASH_ATTR mifareSendWriteBlockCommand(uint8_t blockNumber,
		uint8_t * _data);

static void ICACHE_FLASH_ATTR mifareSendWriteCMD();

static uint8_t ICACHE_FLASH_ATTR masterCard(uint8_t *cardUid,
		uint8_t uidLength);

static bool ICACHE_FLASH_ATTR checkUID(uint8_t * uid, uint8_t length);

static bool ICACHE_FLASH_ATTR mifareclassic_IsTrailerBlock (uint32_t uiBlock);

static bool ICACHE_FLASH_ATTR mifareclassic_IsFirstBlock (uint32_t uiBlock);

static void ICACHE_FLASH_ATTR enablePn532Interrupt();

static void ICACHE_FLASH_ATTR disablePn532Interrupt();

///////////////////////////////////////////////////////
bool ICACHE_FLASH_ATTR pn532ReadAuthenticateResponse();

bool ICACHE_FLASH_ATTR pn532MifareReadDataBlock();

bool ICACHE_FLASH_ATTR pn532MifareReadAckFrame();

bool ICACHE_FLASH_ATTR pn532ReadWritenDataFromBlock();

bool ICACHE_FLASH_ATTR arraycmp(uint8_t *firstArray, uint8_t *secondArray,
		uint8_t _length);

bool ICACHE_FLASH_ATTR pn532_readGpioResp();

bool ICACHE_FLASH_ATTR pn532_led_color(_LEDColor led_color);

///////////////////////////////////////////
void ICACHE_FLASH_ATTR pn532Connection();

void ICACHE_FLASH_ATTR pn532ReadCardUid();

void ICACHE_FLASH_ATTR printlnArrayHex(uint8_t * _array, uint8_t _length);

void ICACHE_FLASH_ATTR printlnArrayHex32(uint32 * _array, uint32 _length);

void ICACHE_FLASH_ATTR printlnArrayChar(uint8_t * _array, uint8_t _length);

void ICACHE_FLASH_ATTR printlnArrayChar32(uint32 * _array, uint32 _length);

void ICACHE_FLASH_ATTR flushArray(uint8_t * _array, uint8_t _length);

void ICACHE_FLASH_ATTR flushArray32(uint32 * _array, int _length);



