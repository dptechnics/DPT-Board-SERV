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
 * File:   spi.c
 * Created on August 26, 2014, 11:00 AM
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include "spi.h"
#include "../config.h"

/* Initialise the SPI device.
 * @mode select the SPI mode (mode 0 = 0b00, mode 1 = 0b01, mode 2 = 0b01, mode 3 = 0b11)
 * @bits the number of bits to read from the device
 *
 * @return the SPI device file descriptor or -1 on error.
 */
int spi_init(uint32_t mode, uint8_t bits, uint32_t speed)
{
	/* The SPI device file descriptor */
	int dev;

	/* Try to open the SPI device */
	dev = open(SPI_DEVICE, O_RDWR);
	if(dev < 0) {
		return -1;
	}

	/* Set the SPI mode */
	if(ioctl(dev, SPI_IOC_WR_MODE, &mode) == -1) {
		return -1;
	}

	/* Set the number of bits per word */
	if(ioctl(dev, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) {
		return -1;
	}

	/* Set the maximum bus speed in Hz */
	if(ioctl(dev, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
		return -1;
	}

	return dev;
}

/**
 * Send a byte to the SPI device
 * @dev the SPI device to use.
 * @byte the byte to send
 */
int spi_byte_send_8(int dev, uint8_t byte)
{
	uint32_t speed;
	uint8_t bits;

	/* Temporary receive buffer */
	uint8_t rx;

	/* Get speed settings for the device */
	if(ioctl(dev, SPI_IOC_RD_MAX_SPEED_HZ, &speed) == -1) {
		return -1;
	}

	/* Get bits per word settings for the device */
	if(ioctl(dev, SPI_IOC_RD_BITS_PER_WORD, &bits) == -1) {
		return -1;
	}

	/* Prepare the transfer structure */
	struct spi_ioc_transfer tr = {
			.tx_buf = (unsigned long) &byte,
			.rx_buf = (unsigned long) &rx,
			.len = 1,
			.delay_usecs = 0,
			.speed_hz = speed,
			.bits_per_word = bits,
	};

	/* Try to transfer the data */
	if(ioctl(dev, SPI_IOC_MESSAGE(1), &tr) < 1){
		return -1;
	}

	/* Close the SPI device */
	close(dev);

	/* Transfer complete */
	return 0;
}

/**
 * Send data of maximum 8 bits to the SPI device.
 * @dev the SPI device to use.
 * @data an array of bytes to send.
 * @size the number of bytes to send.
 */
int spi_data_send_8(int dev, uint8_t data[], int size)
{
	uint32_t speed;
	uint8_t bits;

	/* Make a receive buffer ready of the same length of send buffer */
	uint8_t *rx = (uint8_t*) malloc(size*sizeof(uint8_t));

	/* Get speed settings for the device */
	if(ioctl(dev, SPI_IOC_RD_MAX_SPEED_HZ, &speed) == -1) {
		return -1;
	}

	/* Get bits per word settings for the device */
	if(ioctl(dev, SPI_IOC_RD_BITS_PER_WORD, &bits) == -1) {
		return -1;
	}

	/* Prepare the transfer structure */
	struct spi_ioc_transfer tr = {
			.tx_buf = (unsigned long) data,
			.rx_buf = (unsigned long) rx,
			.len = size,
			.delay_usecs = 0,
			.speed_hz = speed,
			.bits_per_word = bits,
	};

	/* Try to transfer the data */
	if(ioctl(dev, SPI_IOC_MESSAGE(1), &tr) < 1){
		return -1;
	}

	/* Close the SPI device */
	close(dev);

	/* Free the rx buffer */
	free(rx);

	/* Transfer complete */
	return 0;
}

/**
 * Send data bigger than 8 bits to the SPI device.
 * @dev the SPI device to use.
 * @data an array of bytes to send.
 * @size the number of bytes to send.
 */
int spi_data_send_16(int dev, uint16_t data[], int size)
{
	uint32_t speed;
	uint8_t bits;

	/* Make a receive buffer ready of the same length of send buffer */
	uint16_t *rx = (uint16_t*) malloc(size*sizeof(uint16_t));

	/* Get speed settings for the device */
	if(ioctl(dev, SPI_IOC_RD_MAX_SPEED_HZ, &speed) == -1) {
		return -1;
	}

	/* Get bits per word settings for the device */
	if(ioctl(dev, SPI_IOC_RD_BITS_PER_WORD, &bits) == -1) {
		return -1;
	}

	/* Prepare the transfer structure */
	struct spi_ioc_transfer tr = {
			.tx_buf = (unsigned long) data,
			.rx_buf = (unsigned long) rx,
			.len = size,
			.delay_usecs = 0,
			.speed_hz = speed,
			.bits_per_word = bits,
	};

	/* Try to transfer the data */
	if(ioctl(dev, SPI_IOC_MESSAGE(1), &tr) < 1){
		return -1;
	}

	/* Close the SPI device */
	close(dev);

	/* Free the rx buffer */
	free(rx);

	/* Transfer complete */
	return 0;
}

/**
 * Read data of maximum 8 bits from the SPI device.
 * @dev the SPI device to use.
 * @size the number of bytes to read.
 */
spidata_8* spi_data_read_8(int dev, uint32_t size)
{
	uint32_t speed;
	uint8_t bits;

	/* Prepare the spidata structure */
	spidata_8 *data = (spidata_8*) malloc(sizeof(spidata_8));
	data->data = (uint8_t*) calloc(size, sizeof(uint8_t));
	data->size = size;

	/* Prepare empty data transmit buffer */
	uint8_t *tx = (uint8_t*) calloc(size, sizeof(uint8_t));

	/* Get speed settings for the device */
	if(ioctl(dev, SPI_IOC_RD_MAX_SPEED_HZ, &speed) == -1) {
		return NULL;
	}

	/* Get bits per word settings for the device */
	if(ioctl(dev, SPI_IOC_RD_BITS_PER_WORD, &bits) == -1) {
		return NULL;
	}

	/* Prepare the transfer structure */
	struct spi_ioc_transfer tr = {
			.tx_buf = (unsigned long) tx,
			.rx_buf = (unsigned long) data->data,
			.len = size,
			.delay_usecs = 0,
			.speed_hz = speed,
			.bits_per_word = bits,
	};

	/* Try to transfer the data */
	if(ioctl(dev, SPI_IOC_MESSAGE(1), &tr) < 1){
		return NULL;
	}

	/* Close the SPI device */
	close(dev);

	/* Free the rx buffer */
	free(tx);

	/* Return the data */
	return data;
}

/**
 * Read data bigger than 8 bits from the SPI device.
 * @dev the SPI device to use.
 * @size the number of bytes to read.
 */
spidata_16* spi_data_read_16(int dev, uint32_t size)
{
	uint32_t speed;
	uint8_t bits;

	/* Prepare the spidata structure */
	spidata_16 *data = (spidata_16*) malloc(sizeof(spidata_16));
	data->data = (uint16_t*) calloc(size, sizeof(uint16_t));
	data->size = size;

	/* Prepare empty data transmit buffer */
	uint16_t *tx = (uint16_t*) calloc(size, sizeof(uint16_t));

	/* Get speed settings for the device */
	if(ioctl(dev, SPI_IOC_RD_MAX_SPEED_HZ, &speed) == -1) {
		return NULL;
	}

	/* Get bits per word settings for the device */
	if(ioctl(dev, SPI_IOC_RD_BITS_PER_WORD, &bits) == -1) {
		return NULL;
	}

	/* Prepare the transfer structure */
	struct spi_ioc_transfer tr = {
			.tx_buf = (unsigned long) tx,
			.rx_buf = (unsigned long) data->data,
			.len = size,
			.delay_usecs = 0,
			.speed_hz = speed,
			.bits_per_word = bits,
	};

	/* Try to transfer the data */
	if(ioctl(dev, SPI_IOC_MESSAGE(1), &tr) < 1){
		return NULL;
	}

	/* Close the SPI device */
	close(dev);

	/* Free the rx buffer */
	free(tx);

	/* Return the data */
	return data;
}
