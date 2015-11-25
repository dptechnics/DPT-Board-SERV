/* 
 * Copyright (c) 2014, Daan Pape
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
 * File:   spi.h
 * Created on August 26, 2014, 11:00 AM
 */

#ifndef SPI_H_
#define SPI_H_

#include <stdint.h>

/* SPI data structure for maximum 8 bit words */
typedef struct spi_data_8 {
	uint32_t size;
	uint8_t* data;
} spidata_8;

/* SPI data structure for more than 8 bit words*/
typedef struct spi_data_16 {
	uint32_t size;
	uint16_t* data;
} spidata_16;

/**
 * Initialise the SPI device.
 * @mode select the SPI mode (mode 0 = 0b00, mode 1 = 0b01, mode 2 = 0b01, mode 3 = 0b11)
 * @bits the number of bits to read from the device
 */
int spi_init(uint32_t mode, uint8_t bits, uint32_t speed);

/**
 * Send a byte to the SPI device
 * @dev the SPI device to use.
 * @byte the byte to send
 */
int spi_byte_send_8(int dev, uint8_t byte);

/**
 * Send data of maximum 8 bits to the SPI device.
 * @dev the SPI device to use.
 * @data an array of bytes to send.
 * @size the number of bytes to send.
 */
int spi_data_send_8(int dev, uint8_t data[], int size);

/**
 * Send data bigger than 8 bits to the SPI device.
 * @dev the SPI device to use.
 * @data an array of bytes to send.
 * @size the number of bytes to send.
 */
int spi_data_send_16(int dev, uint16_t data[], int size);

/**
 * Read data of maximum 8 bits from the SPI device.
 * @dev the SPI device to use.
 * @size the number of bytes to read.
 */
spidata_8* spi_data_read_8(int dev, uint32_t size);

/**
 * Read data bigger than 8 bits from the SPI device.
 * @dev the SPI device to use.
 * @size the number of bytes to read.
 */
spidata_16* spi_data_read_16(int dev, uint32_t size);

#endif /* SPI_H_ */
