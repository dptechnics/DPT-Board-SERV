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
 * File:   tempsensor.c
 * Created on February 9, 2015, 10:40 PM
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "../logger.h"
#include "tempsensor.h"

/**
 * Get the current temperature from a given temperature sensor. 
 * @param sensor the sensor to read. 
 * @return the current temperature
 */
float tempsensor_current_temp(int sensor)
{
    int fd;		/* File descriptor for the temperature sensor */
    char buf[80];       /* Write buffer */
    int raw;            /* Temperature in RAW form */
    float temp;         /* Temperature in degrees celcius */

    /* Try to open GPIO port */
    fd = open("/sys/devices/w1_bus_master1/28-000003ea41b5/w1_slave", O_RDONLY);
    if(fd < 0) {
        /* The file could not be opened */
        return -1;
    }

    /* Read the port state */
    if(read(fd, buf, 75) < 0) {
            return -1;
    }
    buf[79] = '\0';
    log_message(LOG_DEBUG, "Read '%s' from temperature sensor\r\n", buf);
    
    /* Close the temperature sensor */
    if(close(fd) < 0) {
            return -1;
    }

    /* Read temperature */
    sscanf(buf+69, "%d", &raw);
    temp = (float) raw/1000.0;

    /* Return the state */
    return temp;
}

