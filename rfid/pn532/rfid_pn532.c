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
 * File:   rfid_pn532.c
 * Created on December 7, 2015, 04:00 PM
 */

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "../../combus/i2c.h"
#include "../../gpio/gpio.h"
#include "../../logger.h"
#include "rfid_pn532.h"

/* Driver global variables */
uint8_t pn532ack[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
uint8_t pn532response_firmwarevers[] = {0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03};
uint8_t pn532_packetbuffer[RFID_PN532_PACKBUFFSIZE];
int _irq = -1;

/**
 * Initialize the PN532 rfid card reader to work
 * over the I2C-bus. 
 * @param sda the GPIO pin used for SDA.
 * @param scl the GPIO pin used for SCL.
 * @param irq the GPIO pin used for IRQ.
 * @param reset the GPIO pin used for RESET (active low)
 * @return true on success, false on error. 
 */
bool rfid_pn532_init_i2c(int sda, int scl, int irq, int reset)
{
    log_message(LOG_DEBUG, "Initializing RFID_PN532 reader (SDA: %d, SCL: %d, IRQ: %d)\r\n", sda, scl, irq);
    _irq = irq;
    
    // Set the IRQ line as an input
    /* Reserve the port */
    if (!gpio_reserve(irq)) {
        log_message(LOG_ERROR, "rfid_pn532_init_i2c: could not claim ownership of IRQ port: %d\r\n", irq);
        return false;
    }
    
    if(!gpio_set_direction(irq, GPIO_IN)) {
        log_message(LOG_ERROR, "rfid_pn532_init_i2c: could not set-up IRQ port as an input\r\n");
        return false;
    }

    if (!gpio_release(irq)) {
        log_message(LOG_ERROR, "rfid_pn532_init_i2c: could not release ownership of IRQ port: %d\r\n", irq);
        return false;
    }
    
    
    // Try to enable the I2C bus, this will also fail when the device is already enabled
    // so we don't make this a failure point. 
    if(!i2c_enable_device(RFID_PN532_I2C_BUSNO, sda, scl)) {
        log_message(LOG_WARNING, "rfid_pn532_init_i2c: could not enable the I2C device in the kernel, maybe allready enabled?\r\n");
    }
    
    // Try to open the I2C bus for reading and writing
    if(!i2c_open_bus(RFID_PN532_I2C_BUSNO)) {
        log_message(LOG_ERROR, "rfid_pn532_init_i2c: could not open the I2C bus for reading/writing\r\n");
        return false;
    }
    
    // Assign the slave address 
    if(!i2c_set_slave_address(RFID_PN532_I2C_BUSNO, RFID_PN532_I2C_ADDRESS)) {
        log_message(LOG_ERROR, "rfid_pn532_init_i2c: could not assign slave address to I2C bus\r\n");
        i2c_close_bus(RFID_PN532_I2C_BUSNO);
    }
    
    // Perform device reset
    if(!gpio_write_and_close(reset, GPIO_LOW)) {
        goto bad;
    }
    if(!gpio_write_and_close(reset, GPIO_HIGH)) {
        goto bad;
    }
    usleep(400000);
    if(!gpio_write_and_close(reset, GPIO_LOW)) {
        goto bad;
    }
    usleep(10000);
    
    return true;

bad:
    log_message(LOG_ERROR, "rfid_pn532_init_i2c: could not control reset GPIO's\r\n");
    i2c_close_bus(RFID_PN532_I2C_BUSNO);
    return false;
}

/**
 * Write a command to the PN532 reader. The preamble, postamble
 * and checksum are also calculated and sent. 
 * @param cmd the command buffer. 
 * @param cmdlen the number of payload data bytes. 
 * @return true on success.
 */
bool rfid_pn532_write_command(uint8_t *cmd, uint8_t cmdlen)
{
    uint8_t outbuf[cmdlen + 8];
    int outindex = 0;
    
    log_message(LOG_DEBUG, "Writing RFID_PN532 command: [ ");
    for(int i = 0; i < cmdlen; ++i) {
        printf("%02x ", cmd[i]);
    }
    printf("]\r\n");
    uint8_t checksum = 0;
    ++cmdlen;
    
    // Wake up delay
    usleep(2000);
    
    // Send preamble and data length
    checksum = RFID_PN532_PREAMBLE + RFID_PN532_PREAMBLE + RFID_PN532_STARTCODE2;
    outbuf[outindex++] = RFID_PN532_PREAMBLE;
    outbuf[outindex++] = RFID_PN532_PREAMBLE;
    outbuf[outindex++] = RFID_PN532_STARTCODE2;
    
    outbuf[outindex++] = cmdlen;
    outbuf[outindex++] = ~cmdlen + 1;
    
    checksum += RFID_PN532_HOSTTOPN532;
    outbuf[outindex++] = RFID_PN532_HOSTTOPN532;

    // Send payload data
    for (int i = 0; i < cmdlen - 1; ++i) {
        checksum += cmd[i];
        outbuf[outindex++] = cmd[i];
    }

    // Send postamble
    outbuf[outindex++] = ~checksum;
    outbuf[outindex++] = RFID_PN532_POSTAMBLE;
    
    // Send the payload
    return i2c_write_bytes(RFID_PN532_I2C_BUSNO, outbuf, cmdlen + 8);
}

/**
 * Read n bytes of data from the PN532. 
 * @param buf the buffer where the data will be written.
 * @param n the number of bytes to be read. 
 * @return true on success, false on error.
 */
bool rfid_pn532_read_data(uint8_t* buf, uint8_t n)
{
    int rawlen = (int) n + 2;   /* number of raw bytes to read */
    uint8_t raw[rawlen];        /* raw data buffer */
    int read = 0;               /* number of read bytes */
    int r = 0;                  /* result of read command */
    
    usleep(2000);
    
    while(read != rawlen) {
        r = i2c_read(RFID_PN532_I2C_BUSNO, raw + read, rawlen);
        if(r == -1) {
            log_message(LOG_DEBUG, "Got error while reading from the I2C bus\r\n");
            return false;
        }
        read += r;
    }
    
    // Discard leading 0x01 and trailing 0x00
    log_message(LOG_DEBUG, "rfid_pn532_read_data: [ ");
    for(int i = 0; i < n; ++i) {
        buf[i] = raw[i+1];
        printf("%02x ", buf[i]);
    }
    printf("]\r\n");
    
    return true;
}

/**
 * Returns true if the PN532 is ready with a response. This 
 * is the case when the IRQ pin is pulled low. 
 * @return true if the PN532 is ready. 
 */
bool rfid_pn532_is_ready() {
    return gpio_read_and_close(_irq) == GPIO_LOW;
}

/**
 * Waits until the PN532 is ready. 
 * @param timeout the number of milliseconds before giving up. 
 * @return true when the device is ready within timeout.
 */
bool rfid_pn532_waitready(uint16_t timeout) {
    uint16_t timer = 0;
    
    while(!rfid_pn532_is_ready()) {
        if (timeout != 0) {
            timer += 1;
            if (timer > timeout) {
                log_message(LOG_ERROR, "RFID_PN532 timed out while waiting to be ready\r\n");
                return false;
            }
        }
        usleep(1000);
    }
    return true;
}

/**
 * Tries to read the ACK signal. 
 * @return true on success, false on error. 
 */
bool rfid_pn532_readack() {
    uint8_t ackbuf[6];
    
    if(rfid_pn532_read_data(ackbuf, 6)) {
        return (strncmp((char*) ackbuf, (char*) pn532ack, 6) == 0);
    }
    
    log_message(LOG_ERROR, "RFID_PN532 readack failed\r\n");
    return false;
}

/**
 * Send a command to the PN532 RFID reader and wait
 * for an ACK or error. A timeout must be specified after
 * which the i2c master gives up. 
 * @param command the command byte buffer to send to the PN532
 * @param cmdlen the number of bytes in the payload buffer.
 * @param timeout the number of milliseconds before timeout. 
 * @return true when the reader sent an ACK.
 */
bool rfid_pn532_send_command_check_ack(uint8_t *cmd, uint8_t cmdlen, uint16_t timeout)
{   
    // Send the command
    rfid_pn532_write_command(cmd, cmdlen);
    
    // Wait for the chip to say its ready
    if(!rfid_pn532_waitready(timeout)) {
        return false;
    }
    
    // Read ack
    return rfid_pn532_readack();
}

/**
 * Get the PN532 embedded firmware version. 
 * @return the firmware version or -0 on error.
 */
uint32_t rfid_pn532_get_firmware_version()
{
    uint32_t response;
    
    pn532_packetbuffer[0] = RFID_PN532_COMMAND_GETFIRMWAREVERSION;
    
    if(!rfid_pn532_send_command_check_ack(pn532_packetbuffer, 1, RFID_PN532_ACK_TIMEOUT)) {
        log_message(LOG_ERROR, "rfid_pn532_get_firmware_version: could not send command\r\n");
        return 0;
    }
    
    if(!rfid_pn532_read_data(pn532_packetbuffer, 12)) {
        log_message(LOG_ERROR, "rfid_pn532_get_firmware_version: could not read 12 data bytes\r\n");
        return 0;
    }
    
    if(strncmp((char*) pn532_packetbuffer, (char*) pn532response_firmwarevers, 6)) {
        log_message(LOG_ERROR, "rfid_pn532_get_firmware_version: the firmware packet does not match\r\n");
        return 0;
    }
    
    int offset = 7;
    response = pn532_packetbuffer[offset++];
    response <<= 8;
    response |= pn532_packetbuffer[offset++];
    response <<= 8;
    response |= pn532_packetbuffer[offset++];
    response <<= 8;
    response |= pn532_packetbuffer[offset++];
    
    return response;
}

/**
 * Configure the SAM (Secure Access Module). 
 * @return true on success. 
 */
bool rfid_pn532_config_SAM()
{
    pn532_packetbuffer[0] = RFID_PN532_COMMAND_SAMCONFIGURATION;
    pn532_packetbuffer[1] = 0x01;   // normal mode
    pn532_packetbuffer[2] = 0x14;   // timeout 50ms*20 = 1 second
    pn532_packetbuffer[3] = 0x01;   // use IRQ pin
    
    if(!rfid_pn532_send_command_check_ack(pn532_packetbuffer, 4, RFID_PN532_ACK_TIMEOUT)) {
        log_message(LOG_ERROR, "rfid_pn532_config_SAM: could not send command\r\n");
        return false;
    }
    
    if(!rfid_pn532_read_data(pn532_packetbuffer, 8)) {
        log_message(LOG_ERROR, "rfid_pn532_config_SAM: could not read 8 data bytes\r\n");
        return false;
    }
    
    return pn532_packetbuffer[6] == 0x15;
}

/**
 * Sets the MxRtyPassiveActivation byte of the RFConfiguration register.
 * @param max_retries 0xFF to wait forever, 0x00..0xFE to timeout after mxRetries.
 * @return true on success, false on error.
 */
bool rfid_pn532_set_passive_activation_retries(uint8_t max_retries) 
{
    pn532_packetbuffer[0] = RFID_PN532_COMMAND_RFCONFIGURATION;
    pn532_packetbuffer[1] = 5;      // Config item 5 (MaxRetries)
    pn532_packetbuffer[2] = 0xFF;   // MxRtyATR (default = 0xFF)
    pn532_packetbuffer[3] = 0x01;   // MxRtyPSL (default = 0x01)
    pn532_packetbuffer[4] = max_retries;
    
    if(!rfid_pn532_send_command_check_ack(pn532_packetbuffer, 5, RFID_PN532_ACK_TIMEOUT)) {
        log_message(LOG_ERROR, "rfid_pn532_set_passive_activation_retries: could not send command\r\n");
        return false;
    }
    
    return true;
}

/**
 * Waits for an ISO14443A target to enter the field. 
 * @param cardbaudrate baud rate of the card. 
 * @param uid pointer to the array that will be populated with the card's UID (up to 7 bytes). 
 * @param uidlen pointer to the variable that will hold the length of the card's UID. 
 * @param timeout the read timeout in milliseconds. 
 * @return true on success, false on error or timeout. 
 */
bool rfid_pn532_read_passive_target_id(uint8_t cardbaudrate, uint8_t *uid, uint8_t *uidlen, uint16_t timeout) 
{
    pn532_packetbuffer[0] = RFID_PN532_COMMAND_INLISTPASSIVETARGET;
    pn532_packetbuffer[1] = 1;  // max 1 cards at once (we can set this to 2 later)
    pn532_packetbuffer[2] = cardbaudrate;
    
    if(!rfid_pn532_send_command_check_ack(pn532_packetbuffer, 3, timeout))
    {
        log_message(LOG_ERROR, "rfid_pn532_read_passive_target_id: could not send command\r\n");
        return false;
    }
    
    // Wait for a card to enter the field
    if(!rfid_pn532_waitready(timeout)) {
        log_message(LOG_ERROR, "rfid_pn532_read_passive_target_id: reading tag timed out\r\n");
        return false;
    }
    
    // Read data packet
    if(!rfid_pn532_read_data(pn532_packetbuffer, 20)) {
        log_message(LOG_ERROR, "rfid_pn532_read_passive_target_id: could not read 20 data bytes\r\n");
        return false;
    }
    
    /* 
     * ISO14443A card response should be in the following format:
     * 
     * byte            Description
     * -------------   ------------------------------------------
     * b0..6           Frame header and preamble
     * b7              Tags Found
     * b8              Tag Number (only one used in this example)
     * b9..10          SENS_RES
     * b11             SEL_RES
     * b12             NFCID Length
     * b13..NFCIDLen   NFCID
     * 
     */
    
    // Check if tag was found.
    if(pn532_packetbuffer[7] != 1) {
        log_message(LOG_ERROR, "rfid_pn532_read_passive_target_id: no tag was found\r\n");
        return false;
    }
    
    // Debug information
    uint16_t sens_res = pn532_packetbuffer[9];
    sens_res <<= 8;
    sens_res |= pn532_packetbuffer[10];
    log_message(LOG_DEBUG, "rfid_pn532_read_passive_target_id: ATQA: %#04x, SAK: %#02x\r\n", sens_res, pn532_packetbuffer[11]);
    
    // Read the Mifare Classic UID
    *uidlen = pn532_packetbuffer[12];  
    for (uint8_t i=0; i < pn532_packetbuffer[12]; i++)
    {
        uid[i] = pn532_packetbuffer[13+i];
    }
    
    return true;
}