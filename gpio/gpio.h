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
 * File:   gpio.h
 * Created on June 20, 2014, 4:59 PM
 */

#ifndef GPIO_H_
#define GPIO_H_

#include <stdbool.h>

/* Direction macros */
#define GPIO_OUT	0		/* GPIO output direction */
#define GPIO_IN		1		/* GPIO input direction */

/* State macros */
#define GPIO_HIGH	1		/* GPIO high state */
#define GPIO_LOW	0		/* GPIO low state */

/* GPIO error macro */
#define GPIO_ERR        2               /* When an error has occured */

/* GPIO modes */
#define GPIO_ACT_LOW    0               /* Use GPIO as active low mode */
#define GPIO_ACT_HIGH   1               /* Use GPIO as active high mode */       

/* GPIO layout map */
extern const bool gpio_config[28];

/*
 * Reserve a GPIO for this program's use.
 * @gpio the GPIO pin to reserve.
 * @return true if the reservation was successful.
 */
bool gpio_reserve(int gpio);

/*
 * Release a GPIO after use.
 * @gpio the GPIO pin to release.
 * @return true if the release was successful.
 */
bool gpio_release(int gpio);

/*
 * Set the direction of the GPIO port.
 * @gpio the GPIO pin to release.
 * @direction the direction of the GPIO port.
 * @return true if the direction could be successfully set.
 */
bool gpio_set_direction(int gpio, int direction);

/**
 * Get the direction of a GPIO port.
 * @param gpio the GPIO port to set the direction for. 
 * @return GPIO_IN if input, GPIO_OUT if output. GPIO_ERR when
 * an error occured.
 */
int gpio_get_direction(int gpio);

/*
 * Set the state of the GPIO port.
 * @gpio the GPIO pin to set the state for.
 * @state 1 or 0
 * @return true if the state change was successful.
 */
bool gpio_set_state(int gpio, int state);

/*
 * Get the state of the GPIO port.
 * @gpio the gpio pin to get the state from.
 * @return GPIO_HIGH if the pin is HIGH, GPIO_LOW if the pin is low. GPIO_ERR
 * when an error occured. 
 */
int gpio_get_state(int gpio);

/*
 * Reserve a GPIO and set it as output. Than set the state
 * and release the port.
 * @gpio the GPIO pin to set the state for.
 * @state 1 or 0
 * @return true if the state change was successful.
 */
bool gpio_write_and_close(int gpio, int state);

/*
 * Read the state of the port. The port can be input
 * or output.
 * @gpio the GPIO pin to read from.
 * @return GPIO_HIGH if the pin is HIGH, GPIO_LOW if the pin is low. Output
 * is -1 when an error occured.
 */
int gpio_read_and_close(int gpio);

/**
 * Pulse a GPIO port for a certain number of micro seconcs. 
 * @param gpio the GPIO port to pulse
 * @param useconds the number of micro seconds to pulse the GPIO port
 * @param mode the mode (ACT_LOW, ACT_HIGH)
 * @return true on success 
 */
bool gpio_pulse(int gpio, int useconds, int mode);

#endif /* GPIO_H_ */
