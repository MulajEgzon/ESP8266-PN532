/*
 * PN532ReadWrite.c
 *
 *  Created on: Jun 28, 2016
 *      Author: Egzon Mulaj
 */

//brzo for comunication with pn532 through i2c protocol
#include "gpio.h"
#include "driver/brzo_i2c.h"
#include "spi_flash.h"
#include "gpio_pins.h"
#include "pn532.h"
#include "wifi.h"

void ICACHE_FLASH_ATTR initVariables() {
	wifi.connection = false;
	pn532.authenticateBlock = false;
	pn532.readingBlock = PN532_READING_SSID_BLOCK;
	pn532.booting = true;
	pn532.cmdLength = 0;
	uint8 i;
	for (i = 0; i < 6; i++) {
		pn532.keyUniversal[i] = 0xFF;
	}

	char ssid[] = "PebasysLLC";
	char pswd[] = "Karamela123$";
	for (i = 0; i < os_strlen(ssid); i++) {
		wifi.clientSsid[i] = ssid[i];
	}
	wifi.clientSsid[i] = '\0';
	for (i = 0; i < os_strlen(pswd); i++) {
		wifi.clientPswd[i] = pswd[i];
	}
	wifi.clientPswd[i] = '\0';

	wifi.serverPort[i] = '\0';
	ets_uart_printf("\nssid : %s\n", wifi.clientSsid);
	ets_uart_printf("pswd : %s\n", wifi.clientPswd);
}

uint8_t * ICACHE_FLASH_ATTR commandBuffer(uint8_t* _cmd, uint8_t cmdlen) {
	uint8_t j = 0;
	uint8_t i = 0;
	uint8_t checksum;
	static uint8_t sendcmd[28];		//[25];
	pn532.cmdLength = 0;
	cmdlen++;
	checksum = PN532_PREAMBLE + PN532_PREAMBLE + PN532_STARTCODE2;
	sendcmd[j++] = PN532_PREAMBLE;
	pn532.cmdLength++;
	sendcmd[j++] = PN532_PREAMBLE;
	pn532.cmdLength++;
	sendcmd[j++] = PN532_STARTCODE2;
	pn532.cmdLength++;
	sendcmd[j++] = cmdlen;
	pn532.cmdLength++;
	sendcmd[j++] = (uint8_t) (~cmdlen + 1);
	pn532.cmdLength++;
	sendcmd[j++] = PN532_HOSTTOPN532;
	pn532.cmdLength++;
	checksum += PN532_HOSTTOPN532;
//	#ifdef DEBUG_PN532
//	ets_uart_printf("\nCMD : ");
//	#endif

	for (i = 0; i < cmdlen - 1; i++) {
		sendcmd[i + 6] = _cmd[i];
		j++;
		checksum += _cmd[i];
//		#ifdef DEBUG_PN532
//		ets_uart_printf(" ,0x%02x",_cmd[i]);
//		#endif
	}

	sendcmd[j++] = (~checksum);
	pn532.cmdLength++;
	sendcmd[j++] = PN532_POSTAMBLE;
	pn532.cmdLength++;
	//os_free(_cmd);
	//os_free(i);
	//os_free(j);
	//os_free(checksum);
	//os_free(cmdlen);
	return sendcmd;
}

static void ICACHE_FLASH_ATTR pn532SendAuthenticationKey(uint8_t block) {
	uint8_t authenticate_block;
	if (mifareclassic_IsFirstBlock(block)) {
		authenticate_block = block + 3;
	}
#ifdef PN532_WRITING_DATA
	ets_uart_printf(
			"\nmifareclassic_IsFirstBlock(block) %u, authenticate_block %u\n",
			mifareclassic_IsFirstBlock(block), authenticate_block);
#endif
#ifdef PN532_WRITING_DATA

	ets_uart_printf("Writing in Card........\n");
	os_delay_us(10000);
	_pn532SendAuthenticationKey(pn532.uid, pn532.uidLength,
			authenticate_block, MIFARE_KEY_B, pn532.keyUniversal);
#endif

#ifdef	 PN532_READING_BLOCK
//	os_delay_us(10000);
	_pn532SendAuthenticationKey(pn532.uid, pn532.uidLength, authenticate_block,
			MIFARE_KEY_B, pn532.keyUniversal);
#endif
}

bool ICACHE_FLASH_ATTR _pn532SendAuthenticationKey(uint8_t * uid,
		uint8_t uidLen, uint8_t blockNumber, uint8_t keyNumber,
		uint8_t * keyData) {
	uint8_t i;
	uint8_t _error;
	uint8_t j = 0;
	uint8_t _cmd[16];
	flushArray(pn532.cmd, PN532_I2C_CMD_LENGTH);
#ifdef DEBUG_PN532
	if (((keyNumber) ? MIFARE_CMD_AUTH_B : MIFARE_CMD_AUTH_A)
			== MIFARE_CMD_AUTH_A)
	ets_uart_printf("\nAuthenticating block %u with key A\n", blockNumber);
	else
	ets_uart_printf("\nAuthenticating block %u with key B\n", blockNumber);
#endif

	_cmd[j++] = PN532_COMMAND_INDATAEXCHANGE;
	_cmd[j++] = 1;
	_cmd[j++] = (keyNumber) ? MIFARE_CMD_AUTH_B : MIFARE_CMD_AUTH_A;
	_cmd[j++] = blockNumber;

	for (i = 0; i < 6; i++) {
		_cmd[j++] = keyData[i];
	}
	for (i = 0; i < uidLen; i++) {
		_cmd[j++] = uid[i];
	}
	pn532.cmd = commandBuffer(_cmd, j);
#ifdef DEBUG_PN532
	ets_uart_printf("\nSENDING :");
	printlnArrayHex(pn532.cmd, j + uidLen);
#endif
	pn532_brzo_start_transaction();
	brzo_i2c_write(pn532.cmd, j + pn532.cmdLength, false);
	_error = brzo_i2c_end_transaction();
	pn532IrqInt = IRQ_ACK_FRAME;
	pn532IrqState = GETTING_BLOCK_AUTH;
	if (_error != 0) {
		brzo_i2c_setup(4, 5, 2089);
		return false;
	}
	return true;
}

bool ICACHE_FLASH_ATTR pn532ReadAuthenticateResponse() {
	flushArray(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
	uint8_t _error;
	pn532_brzo_start_transaction();
	brzo_i2c_read(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH, false);
#ifdef DEBUG_PN532
	ets_uart_printf("\n readAuthenticateResponse :  ");
	printlnArrayHex(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
#endif

	if (pn532.recvBuffer[8] != 0x00) {
#ifdef DEBUG_PN532
		ets_uart_printf("\nAuthentication failed\n");
#endif
		brzo_i2c_end_transaction();
		pn532.readingBlock = PN532_READING_SSID_BLOCK;
		loop();
		return false;
	} else {
#ifdef DEBUG_PN532
		ets_uart_printf("\n\nAUTHENTICATION OK\n\n");
		ets_uart_printf("\npn532.AUTHENTICATE_BLOCK = true;\n");
#endif
		pn532.authenticateBlock = true;
	}
	//ets_uart_printf("\n***************************************************************\n");
	//sendCommandCheckAck(pn532_packetbuffer,10+_uidLen);
	_error = brzo_i2c_end_transaction();
	if (_error != 0) {
		brzo_i2c_setup(4, 5, 2089);
		//os_free(_error);
		return false;
	}
	//os_free(_error);
#ifdef PN532_WRITING_DATA
	os_timer_disarm(&pn532Timer);
	os_timer_setfn(&pn532Timer, (os_timer_func_t *) mifareSendWriteCMD,
			NULL);
	os_timer_arm(&pn532Timer, 2, 0);
#endif

#ifdef PN532_READING_BLOCK
	if (pn532.readingBlock == PN532_READING_SSID_BLOCK) {
#ifdef DEBUG_PN532
		ets_uart_printf("\nREADING SSID BLOCK\n");
#endif
		os_timer_disarm(&pn532Timer);
		os_timer_setfn(&pn532Timer,
				(os_timer_func_t *) pn532MifareSendReadBlockCommand,
				PN532_READING_SSID_BLOCK);
		os_timer_arm(&pn532Timer, 10, 0);
	}
#endif

	return true;
}

static bool ICACHE_FLASH_ATTR pn532SamConfig() {
	uint8_t _error = false;
	uint8_t j = 0;
	uint8_t _try = 0;
	uint8_t _cmd[4];
	flushArray(pn532.cmd, PN532_I2C_CMD_LENGTH);
	flushArray(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
	pn532.cmd[j++] = 0x00;
	pn532.cmd[j++] = 0x00;
	pn532.cmd[j++] = 0xFF;
	pn532.cmd[j++] = 0x06;
	pn532.cmd[j++] = 0xFA;
	pn532.cmd[j++] = 0xd4;
	pn532.cmd[j++] = 0x32;
	pn532.cmd[j++] = 0x05;
	pn532.cmd[j++] = 0xFF;
	pn532.cmd[j++] = 0x01;
	pn532.cmd[j++] = 0xFF;
	pn532.cmd[j++] = 0xF6;
	pn532.cmd[j++] = 0x00;
//	#ifdef DEBUG_PN532L
	ets_uart_printf("SENDING: ");
	printlnArrayHex(pn532.cmd, j);
//	#endif
	pn532_brzo_start_transaction();
	brzo_i2c_write(pn532.cmd, j, false);
	flushArray(pn532.cmd, PN532_I2C_BUFFER_LENGTH);
	read_again:
	os_delay_us(2000);
	pn532_brzo_start_transaction();
	brzo_i2c_read(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH, false);
//	#ifdef DEBUG_PN532L
	ets_uart_printf("\nREADING ::");
	printlnArrayHex(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
//	#endif
	if (pn532ReadAckFrame(pn532.recvBuffer, PN532_ACK_BUFFER_LENGTH)) {
		_try++;
		if (_try == 5) {
			//os_free(_error);
			//os_free(j);
			//os_free(_try);
			//os_free(_cmd);
			return false;
		}
		goto read_again;
	}
	flushArray(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
	flushArray(pn532.cmd, PN532_I2C_BUFFER_LENGTH);
	os_delay_us(2000);
	j = 0;
	_cmd[j++] = PN532_COMMAND_SAMCONFIGURATION;
	_cmd[j++] = 0x01;
	_cmd[j++] = 0x14;
	_cmd[j++] = 0x1;
	pn532.cmd = commandBuffer(_cmd, j);
	os_delay_us(30000);
//	#ifdef DEBUG_PN532
	ets_uart_printf("\nSAM CONFIG \nSENDING ::");
	printlnArrayHex(pn532.cmd, j + pn532.cmdLength);
//	#endif
	brzo_i2c_write(pn532.cmd, j + pn532.cmdLength, false);
	//READING
	os_delay_us(30000);
	flushArray(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
	brzo_i2c_read(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH, false);
	//#ifdef DEBUG_PN532L
	ets_uart_printf("\nREADING ::");
	printlnArrayHex(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
//	#endif	//READING
	os_delay_us(30000);
	brzo_i2c_read(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH, false);
//		#ifdef DEBUG_PN532L
	ets_uart_printf("\nREADING  ::");
	printlnArrayHex(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
//		#endif
	_error = brzo_i2c_end_transaction();
	//os_free(_cmd);

	//os_free(j);
	//os_free(_try);
	//os_free(_cmd);
	if (_error != 0) {
		//os_free(_error);
		return false;
	}
	//os_free(_error);
	return true;
}

static uint32_t ICACHE_FLASH_ATTR pn532GetFirmwareVersion(void) {
	uint8_t _error;
	uint32_t response;
	uint8_t j = 0;
	////// 	Sending New CMD  ////////
	uint8_t cmdbuff[3];
	cmdbuff[j++] = PN532_COMMAND_GETFIRMWAREVERSION;
	pn532.cmd = commandBuffer(cmdbuff, j);

//#ifdef DEBUG_PN532
	ets_uart_printf("SENDING: ");
	printlnArrayHex(pn532.cmd, j + pn532.cmdLength);
//#endif
	pn532_brzo_start_transaction();
	brzo_i2c_write(pn532.cmd, j + pn532.cmdLength, false);
	os_delay_us(2000);
	brzo_i2c_read(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH, false);
//#ifdef DEBUG_PN532
	ets_uart_printf("RECIVING: ");
	printlnArrayHex(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
//#endif
	if (!pn532ReadAckFrame(pn532.recvBuffer, PN532_ACK_BUFFER_LENGTH)) {
		ets_uart_printf("brzo_i2c_end_transaction %u\n",
				brzo_i2c_end_transaction());
		//os_free(_error);
		//os_free(response);
		//os_free(j);
		//os_free(cmdbuff);
		return false;
	}
	os_delay_us(2000);
	brzo_i2c_read(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH, false);
//#ifdef DEBUG_PN532
	ets_uart_printf("RECIVING: ");
	printlnArrayHex(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
//#endif
	response = pn532.recvBuffer[8];
	response <<= 8;
	response |= pn532.recvBuffer[9];
	response <<= 8;
	response |= pn532.recvBuffer[10];
	response <<= 8;
	response |= pn532.recvBuffer[11];
	_error = brzo_i2c_end_transaction();
	//os_free(j);
	//os_free(cmdbuff);
	if (_error != 0) {
		brzo_i2c_setup(4, 5, 2089);
		//os_free(_error);
		return false;
	}
	//os_free(_error);
	return response;
}

static void ICACHE_FLASH_ATTR pn532ReadPassiveTargetID(uint8_t cardbaudrate,
		uint8_t * uid, uint8_t * uidLength) {
//	os_delay_us(10000000);
	uint8_t j = 0;
	uint8_t _cmd[10];
	uint8_t _error;
	flushArray(pn532.cmd, PN532_I2C_CMD_LENGTH);
	flushArray(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
	_cmd[j++] = PN532_COMMAND_INLISTPASSIVETARGET;
	_cmd[j++] = 1;
	_cmd[j++] = cardbaudrate;
	pn532.cmd = commandBuffer(_cmd, j);
#ifdef DEBUG_PN532
	ets_uart_printf("\nREAD PASSIVE TARGET ID : ");
	ets_uart_printf("\nSENDING");
	printlnArrayHex(pn532.cmd, j + pn532.cmdLength);
#endif
	pn532_brzo_start_transaction();
	brzo_i2c_write(pn532.cmd, j + pn532.cmdLength, false);
	//pn532IrqInt = IRQ_ACK_FRAME;
	os_delay_us(30000);
	flushArray(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
	brzo_i2c_read(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH, false);
#ifdef DEBUG_PN532
	ets_uart_printf("READING : ");
	printlnArrayHex(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
#endif
	if (!pn532ReadAckFrame(pn532.recvBuffer, PN532_ACK_BUFFER_LENGTH)) {
		//os_free(_error);
		//os_free(j);
		//os_free(_cmd);

		return;
	}
	_error = brzo_i2c_end_transaction();
	//os_free(j);
	//os_free(_cmd);

	if (_error != 0) {
		brzo_i2c_setup(4, 5, 2089);
		ets_uart_printf("\n_error = %u\n", _error);
		//os_free(_error);
		return;
	}
	pn532.uidLength = 0;
//	ETS_GPIO_INTR_ENABLE();
	//os_free(_error);
	pn532IrqInt = IRQ_ACK_FRAME;
//	pn532IrqState = GETTING_CARD;
	return;
}

static bool ICACHE_FLASH_ATTR pn532ReadyToReadPassiveTargetID(
		uint8_t cardbaudrate, uint8_t * uid, uint8_t * uidLength) {
//	os_delay_us(10000000);
	pn532IrqInt = IRQ_ACK_FRAME;
	pn532IrqState = GETTING_CARD;
	uint8_t j;
	uint8_t _cmd[3];
	flushArray(pn532.cmd, PN532_I2C_CMD_LENGTH);
	flushArray(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
	_cmd[j++] = PN532_COMMAND_INLISTPASSIVETARGET;
	_cmd[j++] = 1;
	_cmd[j++] = cardbaudrate;
	uint8_t _error;
	pn532.cmd = commandBuffer(_cmd, j);
#ifdef DEBUG_PN532
	ets_uart_printf("\nREAD PASSIVE TARGET ID : ");
	ets_uart_printf("\nSENDING");
	printlnArrayHex(pn532.cmd, j + pn532.cmdLength);
#endif
	pn532_brzo_start_transaction();
	brzo_i2c_write(pn532.cmd, j + pn532.cmdLength, false);
	_error = brzo_i2c_end_transaction();

	//os_free(j);
	//os_free(_cmd);

	if (_error != 0) {
		//os_free(_error);
		pn532IrqInt = IRQ_IDLE;
		pn532IrqState = STATUS_IDLE;
		brzo_i2c_setup(4, 5, 2089);
		return false;
	}
	//os_free(_error);

	return true;
}

void ICACHE_FLASH_ATTR pn532ReadCardUid() {
	os_timer_disarm(&esp8266.osLoopTimer);
	os_timer_disarm(&pn532Timer);
	os_timer_disarm(&enableInterruptTimer);
	os_timer_setfn(&enableInterruptTimer,
			(os_timer_func_t *) enablePn532Interrupt, NULL);
	os_timer_arm(&enableInterruptTimer, SECOND_IN_MS, 0);
	uint8_t _error;

	flushArray(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
	pn532_brzo_start_transaction();
	brzo_i2c_read(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH, false);
#ifdef DEBUG_PN532
	ets_uart_printf("\nREAD\t-> ");
	printlnArrayHex(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
#endif
	pn532.uidLength = pn532.recvBuffer[13];
	if (pn532.uidLength > PN532_MAX_UID_LENGTH) {
		ets_uart_printf("ERROR WHILE READING UID MAX_UID_LENGTH : %u\n",
				pn532.uidLength);
		flushArray(pn532.uid, pn532.uidLength);
		pn532.uidLength = 0;
		return;
	}
	uint8_t j = 0;
	for (j = 0; j < pn532.uidLength; j++) {
		pn532.uid[j] = pn532.recvBuffer[14 + j];
	}

	if (checkUID(pn532.uid, pn532.uidLength) == false) {
		ets_uart_printf("ERROR WHILE READING UID\n");
		flushArray(pn532.uid, pn532.uidLength);
		pn532.uidLength = 0;
		//		flushArray()
		return;
	}
	ets_uart_printf(
			"\n-----------------------------------------------------------------------\n");
	ets_uart_printf("UID : ", pn532.uidLength);
	uint8_t i;
	for (i = 0; i < pn532.uidLength; i++) {
		ets_uart_printf("0x");
		ets_uart_printf("%02x", pn532.uid[i]);
		ets_uart_printf(",");
	}
	ets_uart_printf("\n");
	ets_uart_printf("LENGTH : %u\n", pn532.uidLength);

//	uint16_t sens_res = pn532.recvBuffer[8];
//	sens_res <<= 8;
//	sens_res |= pn532.recvBuffer[9];
//	ets_uart_printf("ATQA: 0x%2x\n", sens_res);
//	ets_uart_printf("SAK: 0x%2x\n", pn532.recvBuffer[10]);

#ifdef PN532_WRITING_DATA
	os_timer_disarm(&pn532Timer);
	os_timer_setfn(&pn532Timer,
			(os_timer_func_t *) pn532SendAuthenticationKey, 0);
	os_timer_arm(&pn532Timer, 10, 0);
#endif

#ifdef PN532_READING_BLOCK
	os_timer_disarm(&pn532Timer);
	os_timer_setfn(&pn532Timer, (os_timer_func_t *) pn532SendAuthenticationKey,
			0);
	os_timer_arm(&pn532Timer, 10, 0);

#endif
	_error = brzo_i2c_end_transaction();
	if (_error != 0) {
		ets_uart_printf("\nerror %u\n", _error);
		brzo_i2c_setup(4, 5, 2089);
		pn532Connection();
	}
	os_timer_disarm(&esp8266.osLoopTimer);
	os_timer_setfn(&esp8266.osLoopTimer, (os_timer_func_t *) loop, NULL);
	os_timer_arm(&esp8266.osLoopTimer, SECOND_IN_MS*5, 0);

	return;
}

void ICACHE_FLASH_ATTR enablePn532Interrupt() {
	ets_uart_printf("\nREADY TO READ NEXT UID ...\n");
	pn532ReadyToReadPassiveTargetID(PN532_MIFARE_ISO14443A, pn532.uid,
			&pn532.uidLength);
	gpio_pin_intr_state_set(GPIO_ID_PIN(IRQ_PIN), GPIO_PIN_INTR_ANYEDGE);

}

void ICACHE_FLASH_ATTR disablePn532Interrupt() {
	ets_uart_printf("\ndisablePn532Interrupt\n");
	ETS_GPIO_INTR_DISABLE();
	pn532IrqInt = IRQ_IDLE;
	pn532IrqState = STATION_IDLE;
	gpio_pin_intr_state_set(GPIO_ID_PIN(IRQ_PIN), GPIO_PIN_INTR_DISABLE);
}

void ICACHE_FLASH_ATTR printlnArrayHex(uint8_t * _array, uint8_t _length) {
	if (_length <= 0) {
		//os_free(_array);
		return;
	}
	uint8_t i;
	for (i = 0; i < _length; i++) {
		ets_uart_printf("0x%02x", _array[i]);
		ets_uart_printf(", ");
	}
	ets_uart_printf("\n");
	//os_free(i);
	//os_free(_length);
	//os_free(_array);
}

void ICACHE_FLASH_ATTR printlnArrayHex32(uint32 * _array, uint32 _length) {

	if (_length <= 0) {
		//os_free(_array);
		return;
	}
	uint8_t i;
	for (i = 0; i < _length; i++) {
		ets_uart_printf("0x%02x", _array[i]);
		ets_uart_printf(", ");
	}
	ets_uart_printf("\n");
	//os_free(_array);
	//os_free(i);
	//os_free(_length);
}

void ICACHE_FLASH_ATTR printlnArrayChar(uint8_t * _array, uint8_t _length) {

	if (_length <= 0) {
		//os_free(_array);
		return;
	}
	uint8_t i;
	for (i = 0; i < _length; i++) {
		ets_uart_printf("%c", _array[i]);
		ets_uart_printf(", ");
	}
	ets_uart_printf("\n");
	//os_free(_array);
	//os_free(i);
	//os_free(_length);
}

void ICACHE_FLASH_ATTR printlnArrayChar32(uint32 * _array, uint32 _length) {

	if (_length <= 0) {
		//os_free(_array);
		return;
	}
	uint8_t i;
	for (i = 0; i < _length; i++) {
		ets_uart_printf("%c", _array[i]);
		ets_uart_printf(", ");
	}
	ets_uart_printf("\n");
	//os_free(_array);
	//os_free(i);
	//os_free(_length);
}

void ICACHE_FLASH_ATTR flushArray(uint8_t * _array, uint8_t _length) {
	uint8_t i = 0;
	for (i = 0; i < _length; i++) {
		_array[i] = 0x00;
	}
	//os_free(i);
	//os_free(_length);
	//os_free(_array);
}

void ICACHE_FLASH_ATTR flushArray32(uint32 * _array, int _length) {
	int i = 0;
	for (i = 0; i < _length; i++) {
		_array[i] = 0x00;
	}
	//os_free(i);
	//os_free(_length);
	//os_free(_array);

}

static bool ICACHE_FLASH_ATTR pn532ReadAckFrame(char * _array, uint8_t _length) {
	uint8_t i = 0;
	uint8_t pn532ack[] = { 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00 };

#ifdef DEBUG_PN532L
	ets_uart_printf("_array: ");
	printlnArrayHex(_array,_length+2);
	ets_uart_printf("pn532ack: ");
	printlnArrayHex(pn532ack,_length+1);
#endif
	for (i = 0; i < _length; i++) {
		if (_array[i + 1] != pn532ack[i]) {
			//os_free(_array);
			//os_free(i);
			//os_free(pn532ack);
			//os_free(_length);
			return false;
		}
	}
	//os_free(_array);
	//os_free(i);
	//os_free(pn532ack);
	//os_free(_length);
	return true;
}

static bool ICACHE_FLASH_ATTR pn532MifareSendReadBlockCommand(
		uint8_t blockNumber) {

	uint8_t _error;
	uint8_t _cmd[4];
	uint8_t j = 0;
	flushArray(pn532.cmd, PN532_I2C_CMD_LENGTH);
//	flushArray(pn532.recvBuffer,I2C_BUFFER_LENGTH);
//#ifdef DEBUG_PN532
	ets_uart_printf("\n\t\t\t\t\tBlock %u \n", blockNumber);
//#endif

	_cmd[j++] = PN532_COMMAND_INDATAEXCHANGE;
	_cmd[j++] = 1; // Card number
	_cmd[j++] = MIFARE_CMD_READ; // Mifare Read command = 0x30
	_cmd[j++] = blockNumber; // Block Number (0..63 for 1K, 0..255 for 4K)
	pn532.cmd = commandBuffer(_cmd, j);
#ifdef DEBUG_PN532
	ets_uart_printf("sendReadBlockCommand :");
	printlnArrayHex(pn532.cmd, j + pn532.cmdLength);
#endif
	pn532_brzo_start_transaction();
	brzo_i2c_write(pn532.cmd, j + pn532.cmdLength, false);
	_error = brzo_i2c_end_transaction();
	pn532IrqInt = IRQ_ACK_FRAME;
	pn532IrqState = GETTING_BLOCK_DATA;
	//destroy local variables
	//os_free(_cmd);
	//os_free(j);
	//os_free(blockNumber);
	if (_error != 0) {
		brzo_i2c_setup(4, 5, 2089);
		//os_free(_error);
		return false;
	}
	//os_free(_error);
	return true;
}

bool ICACHE_FLASH_ATTR pn532MifareReadDataBlock() {

	uint8_t _error;
	flushArray(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
	pn532_brzo_start_transaction();
	brzo_i2c_read(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH, false);
#ifdef DEBUG_PN532
	ets_uart_printf("READING DATA:");
	printlnArrayHex(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
	printlnArrayChar(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
#endif

	if (pn532.recvBuffer[8] != 0x00 || pn532.recvBuffer[8] == 0x80) {
		//pn532.CARD_DETECTED = false;
//		pn532.sendReadUidCmd = true;
		pn532IrqInt = IRQ_PASSIVE_TARGET;
#ifdef DEBUG_PN532
		ets_uart_printf("\nUNESXPECTED RESPONSE!\n");
		ets_uart_printf("\nDATA IN BLOCK %u\n", pn532.readingBlock);
		os_memcpy(pn532.dataInBlock, pn532.recvBuffer + 9, 16);
		printlnArrayHex(pn532.dataInBlock, 16);
		printlnArrayChar(pn532.dataInBlock, 16);
#endif
		_error = brzo_i2c_end_transaction();
		ets_uart_printf("\nCALLING LOOP IN ...%u ms\n", SECOND_IN_MS * 1);
		os_timer_disarm(&esp8266.osLoopTimer);
		os_timer_setfn(&esp8266.osLoopTimer, (os_timer_func_t *) loop, NULL);
		os_timer_arm(&esp8266.osLoopTimer, SECOND_IN_MS * 1, 0);
		return false;
	}
	_error = brzo_i2c_end_transaction();
	/* Copy the 16 data bytes to the output buffer        */
	/* Block content starts at byte 9 of a valid response */
	os_memcpy(pn532.dataInBlock, pn532.recvBuffer + 9, 16);

//#ifdef DEBUG_PN532
	ets_uart_printf("\nDATA IN BLOCK %u\n", pn532.readingBlock);
	printlnArrayHex(pn532.dataInBlock, 16);
	ets_uart_printf("%s\n", pn532.dataInBlock);
//	printlnArrayChar(pn532.dataInBlock, 16);
//#endif

	if (_error != 0) {
		brzo_i2c_setup(4, 5, 2089);
		//os_free(_error);
		return false;
	}

	if (pn532.readingBlock == PN532_READING_SSID_BLOCK) {
		pn532.readingBlock = PN532_READING_PSWD_BLOCK;
		pn532.authenticateBlock = false;
		pn532MifareSendReadBlockCommand(pn532.readingBlock);
	} else if (pn532.readingBlock == PN532_READING_PSWD_BLOCK) {
		pn532.readingBlock = PN532_READING_SSID_BLOCK;
		pn532IrqInt = IRQ_PASSIVE_TARGET;
	}
	return true;
}

bool ICACHE_FLASH_ATTR arraycmp(uint8_t *firstArray, uint8_t *secondArray,
		uint8_t _length) {
	uint8_t i = 0;
	for (i = 0; i < _length; i++) {
		if (firstArray[i] != secondArray[i]) {
			//os_free(firstArray);
			//os_free(secondArray);
			//os_free(_length);
			//os_free(i);
			return false;
		}
	}
	//os_free(_length);
	//os_free(i);
	//os_free(firstArray);
	//os_free(secondArray);
	return true;
}

void ICACHE_FLASH_ATTR pn532Connection() {
	if (pn532.booting) {

//		ets_uart_printf("\np532Connection\n");
		uint8_t _try = 0;
		uint32_t version = 0;
#ifdef DEBUG_PN532

		ets_uart_printf("\ngetFirmwareVersion\n");
#endif
		while (_try < 4) {
			version = pn532GetFirmwareVersion();
			if (version == 0) {
				//brzo_i2c_end_transaction();
//				ets_uart_printf("\nversion %u\n", version);

//						LEDStatus(LED_PIN,1,200);
//						LEDStatus(LED_PIN,0,200);
			} else
				break;
			_try++;
		}
		if (version != 0) {
//			ets_uart_printf("\nFound Chip PN5%02x", (version >> 24) & 0xFF);
//			ets_uart_printf("\nFirmware ver.  %u.%u\n", (version >> 16) & 0xFF,(version >> 8) & 0xFF);
			//	os_delay_us(2000);
			if (!pn532SamConfig()) {
				/*os_timer_arm(&timer, 1000, 0);*/
			}
			os_delay_us(2000);

			pn532ReadPassiveTargetID(PN532_MIFARE_ISO14443A, pn532.uid,
					&pn532.uidLength);
			pn532.booting = false;
//			pn532.sendReadUidCmd = true;
			//setIRQPinInterrupt(IRQ_PIN);
			pn532SetIrqPin(IRQ_PIN);
//			pn532Status = PN532_FREE;
			pn532IrqInt = IRQ_IDLE;
			pn532IrqState = STATION_IDLE;
			ets_uart_printf("\n \nCALLING LOOOOPPPP IN %u ms ...\n",
			SECOND_IN_MS);
			os_timer_disarm(&esp8266.osLoopTimer);
			os_timer_setfn(&esp8266.osLoopTimer, (os_timer_func_t *) loop,
			NULL);
			os_timer_arm(&esp8266.osLoopTimer, SECOND_IN_MS, 0);
			os_timer_disarm(&enableInterruptTimer);
			os_timer_setfn(&enableInterruptTimer,
					(os_timer_func_t *) enablePn532Interrupt, NULL);
			os_timer_arm(&enableInterruptTimer, SECOND_IN_MS * 3, 0);
			return;
		} else {
			ets_uart_printf("\n \nCALLING pn532Connection IN %u ms ...\n",
			SECOND_IN_MS);
			os_timer_disarm(&esp8266.osLoopTimer);
			os_timer_setfn(&esp8266.osLoopTimer,
					(os_timer_func_t *) pn532Connection, NULL);
			os_timer_arm(&esp8266.osLoopTimer, SECOND_IN_MS, 0);
		}
	}
}

bool ICACHE_FLASH_ATTR pn532MifareReadAckFrame() {
	uint8_t _error = 0;
#ifdef DEBUG_PN532L
	ets_uart_printf("\nREADING ACK FRAME:");
#endif
	flushArray(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
	pn532_brzo_start_transaction();
	brzo_i2c_read(pn532.recvBuffer, PN532_ACK_BUFFER_LENGTH + 2, false);
	_error = brzo_i2c_end_transaction();

	if (_error != 0) {
		brzo_i2c_setup(4, 5, 2089);
		//os_free(_error);
		return false;
	}
#ifdef DEBUG_PN532L
	printlnArrayHex(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
#endif
	if (!pn532ReadAckFrame(pn532.recvBuffer, PN532_ACK_BUFFER_LENGTH)) {
#ifdef DEBUG_PN532
		ets_uart_printf("\nACKF ERR: pn532IrqInt %u , pn532IrqState %u!\n",
				pn532IrqInt, pn532IrqState);
#endif
		return false;
	}
	switch (pn532IrqState) {
	case WRITING_GPIO_DATA: {
#ifdef DEBUG_PN532L
		ets_uart_printf("\nWRITING_GPIO_DATA\n");
#endif
		pn532IrqInt = IRQ_GPIO_READ_RESP;
		break;
	}
	case GETTING_FIRMWARE: {
#ifdef DEBUG_PN532L
		ets_uart_printf("\nGETTING_FIRMWARE\n");
#endif
		pn532IrqInt = IRQ_FIRMWARE_VERSION;
		break;
	}
	case GETTING_SAMCONFIG: {
#ifdef DEBUG_PN532L
		ets_uart_printf("\nGETTING_SAMCONFIG\n");
#endif
		pn532IrqInt = IRQ_SAMCONFIG;
		break;
	}
	case GETTING_CARD: {
#ifdef DEBUG_PN532L
		ets_uart_printf("\nGETTING_CARD\n");
#endif
		pn532IrqInt = IRQ_READ_UID;
		break;
	}
	case GETTING_BLOCK_DATA: {
#ifdef DEBUG_PN532L
		ets_uart_printf("\nGETTING_BLOCK_DATA\n");
#endif
		pn532IrqInt = IRQ_READ_BLOCK_DATA;
		break;
	}
	case GETTING_BLOCK_AUTH: {
#ifdef DEBUG_PN532L
		ets_uart_printf("\nGETTING_BLOCK_AUTH\n");
#endif
		pn532IrqInt = IRQ_READ_AUTHENTICATION;
		break;
	}
	case WRITING_BLOCK_DATA: {
#ifdef DEBUG_PN532L
		ets_uart_printf("\nWRITING_BLOCK_DATA\n");
#endif
		pn532IrqInt = IRQ_WRITE_DATA;
		break;
	}
	default: {
#ifdef DEBUG_PN532
		ets_uart_printf("\ndefault");
#endif
		break;
	}
	}
	//os_free(_error);
	return true;

}

static bool ICACHE_FLASH_ATTR checkUID(uint8_t * uid, uint8_t length) {
	uint8_t i;
	uint8_t countError = 0;
#ifdef DEBUG_PN532
	ets_uart_printf("Uid length %u\n", length);
//	if(length  == 0)
#endif

	if (length <= 0 || length >= 8) {
		return false;
	}
	if (length != 4 || length != 7) {
		length = 4;
	}
	for (i = 0; i < length; i++) {
		if (uid[i] == 0x80)
			countError++;
	}
	if (countError == length) {
		return false;
	}
	return true;
}

static bool ICACHE_FLASH_ATTR mifareSendWriteBlockCommand(uint8_t blockNumber,
		uint8_t * _data) {

	if (blockNumber == 0) {
		ets_uart_printf("\nCANT WRITE TO BLOCK \'%u\'", blockNumber);
		return false;
	}
	uint8_t _cmd[20];
	uint8_t _error;
	uint8_t j;

	flushArray(pn532.cmd, PN532_I2C_CMD_LENGTH);
//	flushArray(pn532.recvBuffer,I2C_BUFFER_LENGTH);
	_cmd[j++] = PN532_COMMAND_INDATAEXCHANGE;
	_cmd[j++] = 1; /* Card number */
	_cmd[j++] = MIFARE_CMD_WRITE; /* Mifare Write command = 0xA0 */
	_cmd[j++] = blockNumber; /* Block Number (0..63 for 1K, 0..255 for 4K) */
	os_memcpy(_cmd + 4, _data, 16);
	j += 16;
	pn532.cmd = commandBuffer(_cmd, j);
#ifdef DEBUG_PN532
	ets_uart_printf("\nSENDING WRITE COMMAND: ");
	printlnArrayHex(pn532.cmd, PN532_I2C_BUFFER_LENGTH);
	printlnArrayChar(pn532.cmd, PN532_I2C_BUFFER_LENGTH);
#endif
	pn532_brzo_start_transaction();
	brzo_i2c_write(pn532.cmd, j + pn532.cmdLength, false);
	_error = brzo_i2c_end_transaction();
	//os_free(_cmd);
	//os_free(j);
	//os_free(blockNumber);
	//os_free(_data);
	if (_error != 0) {
		brzo_i2c_setup(4, 5, 2089);
		//os_free(_error);
		return false;
	}
	//os_free(_error);
	pn532IrqInt = IRQ_ACK_FRAME;
	pn532IrqState = WRITING_BLOCK_DATA;
	return true;
}

bool ICACHE_FLASH_ATTR pn532ReadWritenDataFromBlock() {
	uint8_t _error;
	pn532_brzo_start_transaction();
	brzo_i2c_read(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH, false);
#ifdef DEBUG_PN532
	ets_uart_printf("READING : ");
	printlnArrayHex(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
	printlnArrayChar(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
	flushArray(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
#endif
	_error = brzo_i2c_end_transaction();
	if (_error != 0) {
		brzo_i2c_setup(4, 5, 2089);
		//os_free(_error);
		return false;
	}
	return true;
}

static void ICACHE_FLASH_ATTR mifareSendWriteCMD() {	//192.168.254.191
	uint8_t block = PN532_WRITING_BLOCK;
	uint8_t _data_[16] = { 'T', 'h', 'i', 's', ' ', 'i', 's', ' ', 'a', ' ',
			't', 'e', 's', 't', '.' };
	ets_uart_printf("\nWriting on  block %u\n", block);
	mifareSendWriteBlockCommand(block, _data_);
	pn532IrqInt = IRQ_ACK_FRAME;
	pn532IrqState = WRITING_BLOCK_DATA;
//	loop();
	return;
}

ICACHE_FLASH_ATTR bool pn532_readGpioResp() {
	LEDColor = LED_COLOR_IDLE;
	uint8_t _error;
	pn532_brzo_start_transaction();
	brzo_i2c_read(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH, false);
#ifdef DEBUG_PN532
	ets_uart_printf("\nreadGpioR\n");
	printlnArrayHex(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
//	printlnArrayChar(pn532.recvBuffer, I2C_BUFFER_LENGTH);
	flushArray(pn532.recvBuffer, PN532_I2C_BUFFER_LENGTH);
#endif

	_error = brzo_i2c_end_transaction();
	if (_error != 0) {
		//os_free(_error);
		brzo_i2c_setup(4, 5, 2089);
		return false;
	}
	os_timer_disarm(&enableInterruptTimer);
	os_timer_setfn(&enableInterruptTimer,
			(os_timer_func_t *) enablePn532Interrupt, NULL);
	os_timer_arm(&enableInterruptTimer, PN532_ENABLE_INTERRUPT_TIMEOUT, 0);
	return true;
}

bool ICACHE_FLASH_ATTR pn532_led_color(_LEDColor led_color) {
	pn532IrqInt = IRQ_ACK_FRAME;
	pn532IrqState = WRITING_GPIO_DATA;

#ifdef DEBUG_PN532
	ets_uart_printf("\npn532_ledGreen\n");
#endif

	uint8_t _error;
	// Make sure pinstate does not try to toggle P32 or P34
	uint8_t pinstate = 0;
	pinstate |= (1 << PN532_GPIO_P32) | (1 << PN532_GPIO_P34);
	// Fill command buffer
	uint8_t _cmd[3];
	uint8_t j = 0;
	_cmd[j++] = PN532_COMMAND_WRITEGPIO;
	_cmd[j] = (PN532_GPIO_VALIDATIONBIT | pinstate);  // P3 Pins
	switch (led_color) {
	case LED_COLOR_NONE:
		_cmd[j] |= (1 << PN532_GPIO_P33);
		_cmd[j++] |= (1 << PN532_GPIO_P31);
#ifdef DEBUG_PN532
		ets_uart_printf("\npn532_led_NONE\n");
#endif
		break;
	case LED_COLOR_ORANGE:
		_cmd[j] |= (0 << PN532_GPIO_P33);
		_cmd[j++] |= (0 << PN532_GPIO_P31);
#ifdef DEBUG_PN532
		ets_uart_printf("\npn532_led_ORANGE\n");
#endif
		break;
	case LED_COLOR_RED:
		_cmd[j] |= (0 << PN532_GPIO_P33);
		_cmd[j++] |= (1 << PN532_GPIO_P31);
#ifdef DEBUG_PN532
		ets_uart_printf("\npn532_led_RED\n");
#endif
		break;
	case LED_COLOR_GREEN:
		_cmd[j] |= (1 << PN532_GPIO_P33);
		_cmd[j++] |= (0 << PN532_GPIO_P31);
#ifdef DEBUG_PN532
		ets_uart_printf("\npn532_led_GREEN\n");
#endif
		break;
	default:
#ifdef DEBUG_PN532
		ets_uart_printf("\npn532_led_IDLE\n");
#endif
		;
	}
	_cmd[j++] = 0x00;    // P7 GPIO Pins (not used ... taken by I2C)
	pn532_brzo_start_transaction();
	flushArray(pn532.cmd, PN532_I2C_CMD_LENGTH);
	pn532.cmd = commandBuffer(_cmd, j);
#ifdef DEBUG_PN532
	ets_uart_printf("\nSENDING : ");
	printlnArrayHex(pn532.cmd, PN532_I2C_CMD_LENGTH);

#endif
	brzo_i2c_write(pn532.cmd, j + pn532.cmdLength, false);

	_error = brzo_i2c_end_transaction();
	if (_error != 0) {
		ets_uart_printf("\n_error: %u\n", _error);
		brzo_i2c_setup(4, 5, 2089);
		//os_free(_error);
		return false;
	}

	return true;

}

static bool ICACHE_FLASH_ATTR mifareclassic_IsTrailerBlock(uint32_t uiBlock) {
	// Test if we are in the small or big sectors
	if (uiBlock < 128)
		return ((uiBlock + 1) % 4 == 0);
	else
		return ((uiBlock + 1) % 16 == 0);
}

static bool ICACHE_FLASH_ATTR mifareclassic_IsFirstBlock(uint32_t uiBlock) {
	// Test if we are in the small or big sectors
	if (uiBlock < 128)
		return ((uiBlock) % 4 == 0);
	else
		return ((uiBlock) % 16 == 0);
}

