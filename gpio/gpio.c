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
 * File:   gpio.c
 * Created on June 20, 2014, 4:59 PM
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../uhttpd.h"
#include "../logger.h"
#include "gpio.h"

/* GPIO configuration, true if GPIO is exposed */
const bool gpio_config[28] = {
    true, /* GPIO 0 */
    true, /* GPIO 1 */
    false, /* GPIO 2 */
    false, /* GPIO 3 */
    false, /* GPIO 4 */
    false, /* GPIO 5 */
    true, /* GPIO 6 */
    true, /* GPIO 7 */
    true, /* GPIO 8 */
    false, /* GPIO 9 */
    false, /* GPIO 10 */
    false, /* GPIO 11 */
    true, /* GPIO 12 */
    true, /* GPIO 13 */
    true, /* GPIO 14 */
    true, /* GPIO 15 */
    true, /* GPIO 16 */
    true, /* GPIO 17 */
    true, /* GPIO 18 */
    true, /* GPIO 19 */
    true, /* GPIO 20 */
    true, /* GPIO 21 */
    true, /* GPIO 22 */
    true, /* GPIO 23 */
    true, /* GPIO 24 */
    false, /* GPIO 25 */
    false, /* GPIO 26 */
    false, /* GPIO 27 */
};

/*
 * Reserve a GPIO for this program's use.
 * @gpio the GPIO pin to reserve.
 * @return true if the reservation was successful.
 */
bool gpio_reserve(int gpio) {
    int fd; /* File descriptor for GPIO controller class */
    char buf[3]; /* Write buffer */

    /* Check if GPIO is valid */
    if (gpio > 27 || !gpio_config[gpio]) {
        return false;
    }

    /* Try to open GPIO controller class */
    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) {
        /* The file could not be opened */
        log_message(LOG_DEBUG, "gpio_reserve: could not open /sys/class/gpio/export\r\n");
        return false;
    }

    /* Prepare buffer */
    sprintf(buf, "%d", gpio);

    /* Try to reserve GPIO */
    if (write(fd, buf, strlen(buf)) < 0) {
        close(fd);
        log_message(LOG_DEBUG, "gpio_reserve: could not write '%s' to /sys/class/gpio/export\r\n", buf);
        return false;
    }

    /* Close the GPIO controller class */
    if (close(fd) < 0) {
        log_message(LOG_DEBUG, "gpio_reserve: could not close /sys/class/gpio/export\r\n");
        return false;
    }

    /* Success */
    return true;
}

/*
 * Release a GPIO after use.
 * @gpio the GPIO pin to release.
 * @return true if the release was successful.
 */
bool gpio_release(int gpio) {
    int fd; /* File descriptor for GPIO controller class */
    char buf[3]; /* Write buffer */

    /* Check if GPIO is valid */
    if (gpio > 27 || !gpio_config[gpio]) {
        return false;
    }

    /* Try to open GPIO controller class */
    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd < 0) {
        /* The file could not be opened */
        log_message(LOG_DEBUG, "gpio_release: could not open /sys/class/gpio/unexport\r\n");
        return false;
    }

    /* Prepare buffer */
    sprintf(buf, "%d", gpio);

    /* Try to release GPIO */
    if (write(fd, buf, strlen(buf)) < 0) {
        log_message(LOG_DEBUG, "gpio_release: could not write /sys/class/gpio/unexport\r\n");
        return false;
    }

    /* Close the GPIO controller class */
    if (close(fd) < 0) {
        log_message(LOG_DEBUG, "gpio_release: could not close /sys/class/gpio/unexport\r\n");
        return false;
    }

    /* Success */
    return true;
}

/*
 * Set the direction of the GPIO port.
 * @gpio the GPIO pin to release.
 * @direction the direction of the GPIO port.
 * @return true if the direction could be successfully set.
 */
bool gpio_set_direction(int gpio, int direction) {
    int fd; /* File descriptor for GPIO port */
    char buf[33]; /* Write buffer */

    /* Check if GPIO is valid */
    if (gpio > 27 || !gpio_config[gpio]) {
        return false;
    }

    /* Make the GPIO port path */
    sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio);

    /* Try to open GPIO port for writing only */
    fd = open(buf, O_WRONLY);
    if (fd < 0) {
        /* The file could not be opened */
        return false;
    }

    /* Set the port direction */
    if (direction == GPIO_OUT) {
        if (write(fd, "out", 3) < 0) {
            return false;
        }
    } else {
        if (write(fd, "in", 2) < 0) {
            return false;
        }
    }

    /* Close the GPIO port */
    if (close(fd) < 0) {
        return false;
    }

    /* Success */
    return true;
}


/**
 * Get the direction of a GPIO port.
 * @param gpio the GPIO port to set the direction for. 
 * @return GPIO_IN if input, GPIO_OUT if output. GPIO_ERR when
 * an error occured.
 */
int gpio_get_direction(int gpio)
{
    int fd; /* File descriptor for GPIO port */
    char buf[33]; /* Write buffer */
    char dir;
    int state;

    /* Check if GPIO is valid */
    if (gpio > 27 || !gpio_config[gpio]) {
        return GPIO_ERR;
    }

    /* Make the GPIO port path */
    sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio);

    /* Try to open GPIO port fro writing only */
    fd = open(buf, O_RDONLY);
    if (fd < 0) {
        /* The file could not be opened */
        log_message(LOG_DEBUG, "gpio_get_direction: could not open /sys/class/gpio/gpio%d/direction\r\n", gpio);
        return GPIO_ERR;
    }
    
    /* Read the port direction */
    if (read(fd, &dir, 1) < 0) {
        close(fd);
        log_message(LOG_DEBUG, "gpio_get_direction: could not read /sys/class/gpio/gpio%d/direction\r\n", gpio);
        return GPIO_ERR;
    }

    /* Translate the port state into API state */
    state = dir == 'i' ? GPIO_IN : GPIO_OUT;

    /* Close the GPIO port */
    if (close(fd) < 0) {
        log_message(LOG_DEBUG, "gpio_get_direction: could not close /sys/class/gpio/gpio%d/direction\r\n", gpio);
        return GPIO_ERR;
    }

    /* Success */
    return state;    
}

/*
 * Set the state of the GPIO port.
 * @gpio the GPIO pin to set the state for.
 * @state 1 or 0
 * @return true if the state change was successful.
 */
bool gpio_set_state(int gpio, int state) {
    int fd; /* File descriptor for GPIO port */
    char buf[29]; /* Write buffer */

    /* Check if GPIO is valid */
    if (gpio > 27 || !gpio_config[gpio]) {
        return false;
    }

    /* Make the GPIO port path */
    sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);

    /* Try to open GPIO port */
    fd = open(buf, O_WRONLY);
    if (fd < 0) {
        /* The file could not be opened */
        return false;
    }

    /* Set the port state */
    if (write(fd, (state == GPIO_HIGH ? "1" : "0"), 1) < 0) {
        return false;
    }

    /* Close the GPIO port */
    if (close(fd) < 0) {
        return false;
    }

    /* Success */
    return true;
}

/*
 * Get the state of the GPIO port.
 * @gpio the gpio pin to get the state from.
 * @return GPIO_HIGH if the pin is HIGH, GPIO_LOW if the pin is low. GPIO_ERR
 * when an error occured. 
 */
int gpio_get_state(int gpio) {
    int fd;             /* File descriptor for GPIO port */
    char buf[29];       /* Write buffer */
    char port_state; /* Character indicating the port state */
    int state; /* API integer indicating the port state */

    /* Check if GPIO is valid */
    if (gpio > 27 || !gpio_config[gpio]) {
        return GPIO_ERR;
    }

    /* Make the GPIO port path */
    sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);

    /* Try to open GPIO port */
    fd = open(buf, O_RDONLY);
    if (fd < 0) {
        /* The file could not be opened */
        log_message(LOG_DEBUG, "gpio_get_state: could not open /sys/class/gpio/gpio%d/value\r\n", gpio);
        return GPIO_ERR;
    }

    /* Read the port state */
    if (read(fd, &port_state, 1) < 0) {
        close(fd);
        log_message(LOG_DEBUG, "gpio_get_state: could not read /sys/class/gpio/gpio%d/value\r\n", gpio);
        return GPIO_ERR;
    }

    /* Translate the port state into API state */
    state = port_state == '1' ? GPIO_HIGH : GPIO_LOW;

    /* Close the GPIO port */
    if (close(fd) < 0) {
        log_message(LOG_DEBUG, "gpio_get_state: could not close /sys/class/gpio/gpio%d/value\r\n", gpio);
        return GPIO_ERR;
    }

    /* Return the state */
    return state;
}

/*
 * Reserve a GPIO and set it as output. Than set the state
 * and release the port.
 * @gpio the GPIO pin to set the state for.
 * @state 1 or 0
 * @return true if the state change was successful.
 */
bool gpio_write_and_close(int gpio, int state) {
    return gpio_reserve(gpio)
            && gpio_set_direction(gpio, GPIO_OUT)
            && gpio_set_state(gpio, state)
            && gpio_release(gpio);
}

/*
 * Read the state of the port. The port can be input
 * or output.
 * @gpio the GPIO pin to read from.
 * @return GPIO_HIGH if the pin is HIGH, GPIO_LOW if the pin is low. Output
 * is -1 when an error occurred.
 */
int gpio_read_and_close(int gpio) {
    int state; /* The port state */

    /* Reserve the port */
    if (!gpio_reserve(gpio)) {
        return -1;
    }

    /* Read the port */
    state = gpio_get_state(gpio);

    if (!gpio_release(gpio)) {
        return -1;
    }

    /* Return the port state */
    return state;
}

/**
 * Arguments for the gpio pulse thread
 */
struct _gpio_pulse_args {
    int port;
    int useconds;
    int mode;
};

/**
 * The entry point of the GPIO pulse thread.
 * @param args the arguments for this entry point. 
 */
static void* _gpio_pulse(void *arguments) {
    struct _gpio_pulse_args *args = (struct _gpio_pulse_args*) arguments;
    
    /* Set port as output */
    gpio_set_direction(args->port, GPIO_OUT);
    
    /* Execute the command */
    if(args->mode == GPIO_ACT_HIGH) {
        gpio_set_state(args->port, GPIO_HIGH);
        usleep(args->useconds);
        gpio_set_state(args->port, GPIO_LOW);
        
    } else {
        gpio_set_state(args->port, GPIO_LOW);
        usleep(args->useconds);
        gpio_set_state(args->port, GPIO_HIGH);
    }
    
    /* Release the port */
    gpio_release(args->port);
    
    /* Free the arguments*/
    free(arguments);
    pthread_exit(NULL);
}

/**
 * Pulse a GPIO port for a certain number of micro seconcs. 
 * @param gpio the GPIO port to pulse
 * @param useconds the number of micro seconds to pulse the GPIO port
 * @param mode the mode (ACT_LOW, ACT_HIGH)
 * @return true on success 
 */
bool gpio_pulse(int gpio, int useconds, int mode)
{
    pthread_t thread;
    struct _gpio_pulse_args *args = (struct _gpio_pulse_args*) calloc(1, sizeof(struct _gpio_pulse_args));

    args->port = gpio;
    args->useconds = useconds;
    args->mode = mode;
    
    /* Reserve the port */
    if(!gpio_reserve(gpio)){
        return false;
    }
    
    /* Start the download */
    if(pthread_create(&thread, NULL, _gpio_pulse, (void*) args) != 0) {
        log_message(LOG_ERROR, "Could not start GPIO pulse thread\r\n");
        return false;
    }
    pthread_detach(thread);
    
    return true;
}
