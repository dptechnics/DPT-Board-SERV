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
c
 */

#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "kunio.h"
#include "../gpio/gpio.h"
#include "../config.h"
#include "../combus/spi.h"

#define BYTETOBINARYPATTERN "%d%d%d%d%d%d%d%d"
#define BYTETOBINARY(byte)  \
  (byte & 0x80 ? 1 : 0), \
  (byte & 0x40 ? 1 : 0), \
  (byte & 0x20 ? 1 : 0), \
  (byte & 0x10 ? 1 : 0), \
  (byte & 0x08 ? 1 : 0), \
  (byte & 0x04 ? 1 : 0), \
  (byte & 0x02 ? 1 : 0), \
  (byte & 0x01 ? 1 : 0)

/*
 * Set the output state of the AlfaIO output modules
 * @state byte array describing the output state of the module.
 * @size the number of bytes to set.
 */
void alfa_set_output(uint8_t state[], uint8_t size)
{
	/* The SPI device pointer */
	int dev;

	/* Disable outputs */
	gpio_write_and_close(ALFA_ENABLE_PORT, 1);

	/* Initialize the SPI device for the AlfaIO module */
	dev = spi_init(0, 8, 2500);

	/* Send the bytes */
	spi_data_send_8(dev, state, size);

	/* Hold time */
	usleep(1);

	/* Strobe the line */
	gpio_write_and_close(ALFA_STROBE_PORT, 1);
	usleep(1);
	gpio_write_and_close(ALFA_STROBE_PORT, 0);
	gpio_write_and_close(ALFA_ENABLE_PORT, 0);
}

/*
 * Read data from the AlfaIO input modules
 * @return alfadata structure with output information
 */
alfadata* alfa_read_input()
{
	/* The SPI device pointer */
	int dev;
	spidata_8 *data;

	/* Initialize the SPI device for the AlfaIO module */
	dev = spi_init(0, 8, 2500);

	/* Enable the clock (active low) */
	gpio_write_and_close(ALFA_CLK_INH_PORT, 0);

	/* Pulse the shift load to low */
	gpio_write_and_close(ALFA_SH_LD_PORT, 0);
	usleep(1);
	gpio_write_and_close(ALFA_SH_LD_PORT, 1);
	usleep(1);

	/* Read the data input */
	data = spi_data_read_8(dev, 1);

	/* DEBUG DEBUG DEBUG */
	printf("received binary: "BYTETOBINARYPATTERN", received dec: %d\r\n", BYTETOBINARY(data->data[0]), data->data[0]);
	/* DEBUG DEBUG DEBUG */

	free(data->data);
	free(data);

	return NULL;
}

/*
 * Set the enable state of the AlfaIO output modules
 * @enable if this is true the outputs will be driven.
 */
void alfa_set_enable(bool enable)
{
	gpio_write_and_close(ALFA_ENABLE_PORT, enable ? 1 : 0);
}
