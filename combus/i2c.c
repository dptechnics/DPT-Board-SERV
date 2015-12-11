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
 * File:   i2c.c
 * Created on December 2, 2015, 8:43 AM
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdint.h>

#include "i2c.h"
#include "../logger.h"

/* 
 * Array containing file descriptors for i2c-buses. This 
 * array should not be accessed directly. 
 */
int _fds[I2C_MAX_BUSES] = {-1};

/*
 * Set an i2c file descriptor. This functions fails when
 * the bus number is out of range.
 * @param busno the i2c bus number to set the file descriptor for. 
 * @param fd the file descriptor to set. 
 * @return true on success, false on error.  
 */
static inline bool _i2c_set_fd(int busno, int fd) 
{
    if(busno >= I2C_MAX_BUSES) {
        return false;
    } else {
        _fds[busno] = fd;
        return true;
    }
}

/*
 * Get an i2c file descriptor. This functions fails when
 * the bus number is out of range. 
 * @param busno the bus number to get the file descriptor from. 
 * @param -10 on error.
 */
static int _i2c_get_fd(int busno) 
{
    if(busno >= I2C_MAX_BUSES) {
        return -10;
    } else {
        return _fds[busno];
    }
}

/**
 * Checks is a bus number is in range. 
 * @param busno the bus number to check. 
 * @return true if the bus number is in range. 
 */
static bool _i2c_busno_in_range(int busno)
{
    return busno < I2C_MAX_BUSES;
}

/**
 * Enable an I2C-bus on 2 GPIO pins.
 * @param busno the I2C bus number.
 * @param sda the GPIO pin onto which the SDA line must be placed. 
 * @param scl the GPIO pin onto which the SCL line pmust be placed. 
 * @return true on success or false on error. 
 */
bool i2c_enable_device(int busno, int sda, int scl)
{
    char buff[256];
    
    // Generate the insmod command
    sprintf(buff, "insmod i2c-gpio-custom bus%d=%d,%d,%d", busno, busno, sda, scl);
    log_message(LOG_DEBUG, "executing: %s\r\n", buff);
    
    // Insert the kernel modules 
    system("insmod i2c-dev > /dev/null");
    return system(buff);
}

/**
 * Open an I2C-bus on this device. This can only be done after 
 * i2c_enable_device was called. 
 * @return true on success, false on error.
 */
bool i2c_open_bus(int busno)
{
    char buff[16];
    int fd;
    
    if(!_i2c_busno_in_range(busno)) {
        log_message(LOG_ERROR, "i2c_open_bus: bus number (%d) is out of range, maximum is %d\r\n", busno, I2C_MAX_BUSES);
        return false;
    }
    
    // Generate the bus file path
    sprintf(buff, "/dev/i2c-%d", busno);

    // Try to open the I2C device 
    fd = open(buff, O_RDWR);
    if(fd < 0) {
        return false;
    }
    
    return _i2c_set_fd(busno, fd);
}

/**
 * Close an open I2C-bus. 
 * @param busno the number of the bus to close. 
 * @return true on success, false on error.
 */
bool i2c_close_bus(int busno)
{
    int fd = _i2c_get_fd(busno);
    if(fd <= 0) {
        log_message(LOG_ERROR, "i2c_close_bus: the bus number (%d) is out of range or the bus is not yet open\r\n", busno);
        return false;
    }
    
    return close(fd) == 0;
}

/**
 * Set-up the bus to read/write from/to a specific slave device
 * address. 
 * @param busno the i2c-bus to use
 * @param address the address to read/write from/to.
 * @return true on success, false on error. 
 */
bool i2c_set_slave_address(int busno, uint8_t address)
{   
    int fd = _i2c_get_fd(busno);
    if(fd <= 0) {
        log_message(LOG_ERROR, "i2c_set_slave_address: the bus number (%d) is out of range or the bus is not yet open\r\n", busno);
        return false;
    }
    return ioctl(fd, I2C_SLAVE, address) >= 0;
}

/**
 * Write bytes to the I2C slave device. 
 * @param busno the i2c-bus to use.
 * @param byte the byte array to write to the slave device. 
 * @param len the number of bytes to write. 
 * @return true on success, false on error. 
 */
bool i2c_write_bytes(int busno, uint8_t* byte, int len)
{
    int fd = _i2c_get_fd(busno);
    if(fd <= 0) {
        log_message(LOG_ERROR, "i2c_write_bytes: the bus number (%d) is out of range or the bus is not yet open\r\n", busno);
        return false;
    }
    
    errno = 0;
    int r = write(fd, byte, len);
    
    if(r == -1) {
        log_message(LOG_ERROR, "I2C write got an error [%d]: %s\r\n", errno, strerror(errno));
        return false;
    }
    
    return r == len;
}

/**
 * Read from the I2C slave device. 
 * @param busno the i2c-bus to use.
 * @param buffer the buffer to place the data in. 
 * @param len the number of bytes to read. 
 * @return the number of bytes read, 0 on EOF and -1 on error. 
 */
int i2c_read(int busno, uint8_t* buffer, int len)
{
    int fd = _i2c_get_fd(busno);
    if(fd <= 0) {
        log_message(LOG_ERROR, "i2c_write_bytes: the bus number (%d) is out of range or the bus is not yet open\r\n", busno);
        return false;
    }
    
    errno = 0;
    int r = read(fd, buffer, len);
    
    if(r == -1){
        log_message(LOG_ERROR, "I2C read got an error [%d]: %s\r\n", errno, strerror(errno));
    }
    
    return r;
}