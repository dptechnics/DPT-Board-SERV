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
 * File:   i2c.h
 * Created on December 2, 2015, 7:55 AM
 */

#ifndef I2C_H
#define	I2C_H

/* The maximum number of I2C-buses */
#define I2C_MAX_BUSES           5

#include <stdbool.h>
#include <stdint.h>
#include <linux/i2c-dev.h>

/**
 * Enable an I2C-bus on 2 GPIO pins.
 * @param busno the I2C bus number.
 * @param sda the GPIO pin onto which the SDA line must be placed. 
 * @param scl the GPIO pin onto which the SCL line pmust be placed. 
 * @return true on success or false on error. 
 */
bool i2c_enable_device(int busno, int sda, int scl);

/**
 * Open an I2C-bus on this device. This can only be done after 
 * i2c_enable_device was called. 
 * @return true on success, false on error.
 */
bool i2c_open_bus(int busno);

/**
 * Close an open I2C-bus. 
 * @param busno the number of the bus to close. 
 * @return true on success, false on error.
 */
bool i2c_close_bus(int busno);

/**
 * Set-up the bus to read/write from/to a specific slave device
 * address. 
 * @param busno the i2c-bus to use
 * @param address the address to read/write from/to.
 * @return true on success, false on error. 
 */
bool i2c_set_slave_address(int busno, uint8_t address);

/**
 * Write bytes to the I2C slave device. 
 * @param busno the i2c-bus to use.
 * @param byte the byte array to write to the slave device. 
 * @param len the number of bytes to write. 
 * @return true on success, false on error. 
 */
bool i2c_write_bytes(int busno, uint8_t* byte, int len);

/**
 * Read from the I2C slave device. 
 * @param busno the i2c-bus to use.
 * @param buffer the buffer to place the data in. 
 * @param len the number of bytes to read. 
 * @return the number of bytes read, 0 on EOF and -1 on error. 
 */
int i2c_read(int busno, uint8_t* buffer, int len);

#endif

