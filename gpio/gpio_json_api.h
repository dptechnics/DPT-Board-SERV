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
 * File:   gpio_json_api.h
 * Created on April 6, 2015, 4:59 PM
 */

#ifndef GPIO_JSON_API_H
#define	GPIO_JSON_API_H

#include <json-c/json.h>
#include "../uhttpd.h"

/**
 * Route all get requests concerning the gpio module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* gpio_get_router(struct client *cl, char *request);

/**
 * Route all post requests concerning the gpio module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* gpio_post_router(struct client *cl, char *request);

/**
 * Route all put requests concerning the gpio module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* gpio_put_router(struct client *cl, char *request);

/**
 * Get the layout of the GPIO ports of the board.
 * @cl the client who made the request
 * @request the request part of the url
 */
json_object* gpio_get_layout(struct client *cl, char *request);

/**
 * Get the layout of the GPIO ports and also the current GPIO port
 * state. 
 * @param cl the client who made the request.
 * @param request the request part of the url. 
 */
json_object* gpio_get_overview(struct client *cl, char *request);

/**
 * Get the states of all GPIO ports. 
 * @param cl the client who made the request.
 * @param request the request part of the url. 
 */
json_object* gpio_get_all_states(struct client *cl, char *request);

/**
 * Get the state of a given GPIO port.
 * @cl the client who made the request.
 * @request the request part of the url.
 */
json_object* gpio_get_status(struct client *cl, char *request);

/**
 * Turn on or of a GPIO port.
 * @cl the client who made the request.
 * @request the request part of the url. 
 */
json_object* gpio_put_status(struct client *cl, char *request);

/**
 * Set-up the direction of a GPIO port. 
 * @param cl the client who made the request.
 * @param request the request part of the url.
 */
json_object* gpio_put_direction(struct client *cl, char *request);

/**
 * Pulse an output for a number of milliseconds.
 * @param cl the client who made the request.
 * @param request the request part of the url. 
 */
json_object* gpio_put_pulse_output(struct client *cl, char *request);
#endif

