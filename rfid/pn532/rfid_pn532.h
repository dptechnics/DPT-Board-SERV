/* 
 * Copyright (c) 2015, Daan Pape
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 *     1. Redistributions of source code must retain the above copyright 
 *        notice, this list of conditions and the following disclaimer.
 *
 *     2. Redistributions in binary form must reproduce the above copyright 
 *        notice, this list of conditions and the following disclaimer in the 
 *        documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 * Based on:
 * 
 * Copyright (c) 2012, Adafruit Industries
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the
 * names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * File:   rfid_pn532.h
 * Created on December 2, 2015, 9:14 AM
 * 
 */

#ifndef RFID_PN532_H
#define	RFID_PN532_H

#define RFID_PN532_PACKBUFFSIZE                  64
#define RFID_PN532_I2C_BUSNO                     0    

#define RFID_PN532_PREAMBLE                      (0x00)
#define RFID_PN532_STARTCODE1                    (0x00)
#define RFID_PN532_STARTCODE2                    (0xFF)
#define RFID_PN532_POSTAMBLE                     (0x00)

#define RFID_PN532_HOSTTOPN532                   (0xD4)
#define RFID_PN532_PN532TOHOST                   (0xD5)

// PN532 Commands
#define RFID_PN532_COMMAND_DIAGNOSE              (0x00)
#define RFID_PN532_COMMAND_GETFIRMWAREVERSION    (0x02)
#define RFID_PN532_COMMAND_GETGENERALSTATUS      (0x04)
#define RFID_PN532_COMMAND_READREGISTER          (0x06)
#define RFID_PN532_COMMAND_WRITEREGISTER         (0x08)
#define RFID_PN532_COMMAND_READGPIO              (0x0C)
#define RFID_PN532_COMMAND_WRITEGPIO             (0x0E)
#define RFID_PN532_COMMAND_SETSERIALBAUDRATE     (0x10)
#define RFID_PN532_COMMAND_SETPARAMETERS         (0x12)
#define RFID_PN532_COMMAND_SAMCONFIGURATION      (0x14)
#define RFID_PN532_COMMAND_POWERDOWN             (0x16)
#define RFID_PN532_COMMAND_RFCONFIGURATION       (0x32)
#define RFID_PN532_COMMAND_RFREGULATIONTEST      (0x58)
#define RFID_PN532_COMMAND_INJUMPFORDEP          (0x56)
#define RFID_PN532_COMMAND_INJUMPFORPSL          (0x46)
#define RFID_PN532_COMMAND_INLISTPASSIVETARGET   (0x4A)
#define RFID_PN532_COMMAND_INATR                 (0x50)
#define RFID_PN532_COMMAND_INPSL                 (0x4E)
#define RFID_PN532_COMMAND_INDATAEXCHANGE        (0x40)
#define RFID_PN532_COMMAND_INCOMMUNICATETHRU     (0x42)
#define RFID_PN532_COMMAND_INDESELECT            (0x44)
#define RFID_PN532_COMMAND_INRELEASE             (0x52)
#define RFID_PN532_COMMAND_INSELECT              (0x54)
#define RFID_PN532_COMMAND_INAUTOPOLL            (0x60)
#define RFID_PN532_COMMAND_TGINITASTARGET        (0x8C)
#define RFID_PN532_COMMAND_TGSETGENERALBYTES     (0x92)
#define RFID_PN532_COMMAND_TGGETDATA             (0x86)
#define RFID_PN532_COMMAND_TGSETDATA             (0x8E)
#define RFID_PN532_COMMAND_TGSETMETADATA         (0x94)
#define RFID_PN532_COMMAND_TGGETINITIATORCOMMAND (0x88)
#define RFID_PN532_COMMAND_TGRESPONSETOINITIATOR (0x90)
#define RFID_PN532_COMMAND_TGGETTARGETSTATUS     (0x8A)

#define RFID_PN532_I2C_ADDRESS                   (0x48 >> 1)
#define RFID_PN532_I2C_READBIT                   (0x01)
#define RFID_PN532_I2C_BUSY                      (0x00)
#define RFID_PN532_I2C_READY                     (0x01)
#define RFID_PN532_I2C_READYTIMEOUT              (20)

#define RFID_PN532_ACK_TIMEOUT                   1000

#define RFID_PN532_MIFARE_ISO14443A              (0x00)

/**
 * Initialize the PN532 rfid card reader to work
 * over the I2C-bus. 
 * @param sda the GPIO pin used for SDA.
 * @param scl the GPIO pin used for SCL.
 * @param irq the GPIO pin used for IRQ.
 * @param reset the GPIO pin used for RESET (active low)
 * @return true on success, false on error. 
 */
bool rfid_pn532_init_i2c(int sda, int scl, int irq, int reset);

/**
 * Write a command to the PN532 reader. The preamble, postamble
 * and checksum are also calculated and sent. 
 * @param cmd the command buffer. 
 * @param cmdlen the number of payload data bytes. 
 * @return true on success.
 */
bool rfid_pn532_write_command(uint8_t *cmd, uint8_t cmdlen);

/**
 * Read n bytes of data from the PN532. 
 * @param buf the buffer where the data will be written.
 * @param n the number of bytes to be read. 
 * @return true on success, false on error.
 */
bool rfid_pn532_read_data(uint8_t* buf, uint8_t n);

/**
 * Returns true if the PN532 is ready with a response. This 
 * is the case when the IRQ pin is pulled low. 
 * @return true if the PN532 is ready. 
 */
bool rfid_pn532_is_ready();

/**
 * Waits until the PN532 is ready. 
 * @param timeout the number of milliseconds before giving up. 
 * @return true when the device is ready within timeout.
 */
bool rfid_pn532_waitready(uint16_t timeout);

/**
 * Tries to read the ACK signal. 
 * @return true on success, false on error. 
 */
bool rfid_pn532_readack();

/**
 * Send a command to the PN532 RFID reader and wait
 * for an ACK or error. A timeout must be specified after
 * which the i2c master gives up. 
 * @param command the command byte buffer to send to the PN532
 * @param cmdlen the number of bytes in the payload buffer.
 * @param timeout the number of milliseconds before timeout. 
 * @return true when the reader sent an ACK.
 */
bool rfid_pn532_send_command_check_ack(uint8_t *cmd, uint8_t cmdlen, uint16_t timeout);

/**
 * Get the PN532 embedded firmware version. 
 * @return the firmware version or -0 on error.
 */
uint32_t rfid_pn532_get_firmware_version();

/**
 * Configure the SAM (Secure Access Module). 
 * @return true on success. 
 */
bool rfid_pn532_config_SAM();

/**
 * Sets the MxRtyPassiveActivation byte of the RFConfiguration register.
 * @param max_retries 0xFF to wait forever, 0x00..0xFE to timeout after mxRetries.
 * @return true on success, false on error.
 */
bool rfid_pn532_set_passive_activation_retries(uint8_t max_retries);

/**
 * Waits for an ISO14443A target to enter the field. 
 * @param cardbaudrate baud rate of the card. 
 * @param uid pointer to the array that will be populated with the card's UID (up to 7 bytes). 
 * @param uidlen pointer to the variable that will hold the length of the card's UID. 
 * @param timeout the read timeout in milliseconds. 
 * @return true on success, false on error or timeout. 
 */
bool rfid_pn532_read_passive_target_id(uint8_t cardbaudrate, uint8_t *uid, uint8_t *uidlen, uint16_t timeout);

#endif

