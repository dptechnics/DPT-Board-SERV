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
 * File:   gpio_json_api.c
 * Created on April 6, 2015, 5:02 PM
 */

#include <stdio.h>
#include <stdio.h>
#include <json-c/json.h>
#include <string.h>
#include <stdlib.h>

#include "../logger.h"
#include "../helper.h"
#include "../uhttpd.h"
#include "gpio_json_api.h"
#include "gpio.h"

/**
 * Route all get requests concerning the gpio module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* gpio_get_router(struct client *cl, char *request) {
    if (helper_str_startswith(request, "layout", 0)) 
    {
        return gpio_get_layout(cl, request + 7);
    } 
    else if (helper_str_startswith(request, "state", 0))
    {
        return gpio_get_status(cl, request + 6);
    }
    else if (helper_str_startswith(request, "overview", 0))
    {
        return gpio_get_overview(cl, request + 9);
    }
    else if (helper_str_startswith(request, "states", 0))
    {
        return gpio_get_all_states(cl, request + 10);
    }
    else
    {
        log_message(LOG_WARNING, "GPIO API got unknown GET request '%s'\r\n", request);
        return NULL;
    }
}

/**
 * Route all post requests concerning the gpio module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* gpio_post_router(struct client *cl, char *request) {
    return NULL;
}

/**
 * Route all put requests concerning the gpio module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* gpio_put_router(struct client *cl, char *request) {
    if (helper_str_startswith(request, "state", 0)) 
    {
        return gpio_put_status(cl, request + 6);
    } 
    else if (helper_str_startswith(request, "dir", 0))
    {
        return gpio_put_direction(cl, request + 4);
    }
    else if (helper_str_startswith(request, "pulse", 0))
    {
        return gpio_put_pulse_output(cl, request + 6);
    }
    else
    {
        log_message(LOG_WARNING, "GPIO API got unknown PUT request '%s'\r\n", request);
        return NULL;
    }
}

/**
 * Get the layout of the GPIO ports of the board.
 * @cl the client who made the request
 * @request the request part of the url
 */
json_object* gpio_get_layout(struct client *cl, char *request) {
    int i;

    /* Create the json object */
    json_object *jobj = json_object_new_object();
    json_object *jarray = json_object_new_array();

    /* Check all the IO ports for existance */
    for(i = 0; i < (sizeof(gpio_config) / sizeof(bool)); ++i) {
        if(gpio_config[i]){
            /* This IO is available, put it in the array */
            json_object *portnr = json_object_new_int(i);
            json_object_array_add(jarray, portnr);
        }
    }

    /* Add the array to the json object */
    json_object_object_add(jobj, "ioports", jarray);

    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
}

/**
 * Get the layout of the GPIO ports and also the current GPIO port
 * state. 
 * @param cl the client who made the request.
 * @param request the request part of the url. 
 */
json_object* gpio_get_overview(struct client *cl, char *request) {
    int i;

    /* Create the json object */
    json_object *jobj = json_object_new_object();
    json_object *jarray = json_object_new_array();

    /* Check the state for every IO port */
    for(i = 0; i < (sizeof(gpio_config) / sizeof(bool)); ++i) {
        if(gpio_config[i]){
            json_object *j_gpio_port = json_object_new_object();
            
            /* Add port number */
            json_object_object_add(j_gpio_port, "number", json_object_new_int(i));
            
            /* Add port direction and state */
            int state = 2;
            int dir = 2;
            
            if(gpio_reserve(i)) {
                state = gpio_get_state(i);
                dir = gpio_get_direction(i);
                gpio_release(i);
            }

            json_object_object_add(j_gpio_port, "state", json_object_new_int(state));
            json_object_object_add(j_gpio_port, "direction", json_object_new_int(dir));
            
            /* Add the port info to the array */
            json_object_array_add(jarray, j_gpio_port);
        }
    }

    /* Add the array to the json object */
    json_object_object_add(jobj, "ports", jarray);

    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
}

/**
 * Get the states of all GPIO ports. 
 * @param cl the client who made the request.
 * @param request the request part of the url. 
 */
json_object* gpio_get_all_states(struct client *cl, char *request){   
    int i;

    /* Create the json object */
    json_object *jobj = json_object_new_object();
    json_object *jarray = json_object_new_array();

    /* Check the state for every IO port */
    for(i = 0; i < (sizeof(gpio_config) / sizeof(bool)); ++i) {
        if(gpio_reserve(i)){
            json_object *j_gpio_port = json_object_new_object();
            
            /* Add data */
            json_object_object_add(j_gpio_port, "port-number", json_object_new_int(i));
            json_object_object_add(j_gpio_port, "port-state", json_object_new_int(gpio_get_state(i)));
            json_object_array_add(jarray, j_gpio_port);
            
            /* Release the port */
            gpio_release(i);
        }
    }

    /* Add the array to the json object */
    json_object_object_add(jobj, "ports", jarray);

    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
}

/**
 * Get the state of a given GPIO port.
 * @cl the client who made the request.
 * @request the request part of the url.
 */
json_object* gpio_get_status(struct client *cl, char *request) 
{
    int gpio_pin;
    int gpio_state;

    /* If sscanf fails the request is malformed */
    if(sscanf(request, "%d", &gpio_pin) != 1) {
        log_message(LOG_WARNING, "GPIO GET status request failed,  bad request");
        cl->http_status = r_bad_req;
        return NULL;
    }

    /* Read the GPIO pin state */
    gpio_state = gpio_read_and_close(gpio_pin);

    /* Check if there was no error reading the pin */
    if(gpio_state == -1){
        cl->http_status = r_error;
        return NULL;
    }

    /* Put data in JSON object */
    json_object *jobj = json_object_new_object();
    json_object *j_pin = json_object_new_int(gpio_pin);
    json_object *j_state = json_object_new_int(gpio_state);

    json_object_object_add(jobj, "pin", j_pin);
    json_object_object_add(jobj, "state", j_state);

    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
}

/**
 * Turn on or of a GPIO port.
 * @cl the client who made the request
 * @request the request part of the url
 */
json_object* gpio_put_status(struct client *cl, char *request) 
{
    /* This functions expects the following request /<gpiopin>/<state> */
    int gpio_pin;
    int gpio_state;

    /* If sscanf fails the request is malformed */
    if (sscanf(request, "%d/%d", &gpio_pin, &gpio_state) != 2) {
        cl->http_status = r_bad_req;
        return NULL;
    }

    if (!gpio_write_and_close(gpio_pin, gpio_state == 1 ? GPIO_HIGH : GPIO_LOW)) {
        cl->http_status = r_error;
        return NULL;
    }

    /* Put data in JSON object */
    json_object *jobj = json_object_new_object();
    json_object *j_pin = json_object_new_int(gpio_pin);
    json_object *j_state = json_object_new_int(gpio_state);

    json_object_object_add(jobj, "pin", j_pin);
    json_object_object_add(jobj, "state", j_state);

    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
}

/**
 * Set-up the direction of a GPIO port. 
 * @param cl the client who made the request.
 * @param request the request part of the url.
 */
json_object* gpio_put_direction(struct client *cl, char *request)
{
    /* This functions expects the following request /<gpiopin>/<direction> */
    int gpio_pin;
    int gpio_direction;

    /* If sscanf fails the request is malformed */
    if(sscanf(request, "%d/%d", &gpio_pin, &gpio_direction) != 2) {
        cl->http_status = r_bad_req;
        return NULL;
    }

    if(!gpio_set_direction(gpio_pin, gpio_direction)) {
        cl->http_status = r_error;
        return NULL;
    }

    /* Put data in JSON object */
    json_object *jobj = json_object_new_object();
    json_object *j_pin = json_object_new_int(gpio_pin);
    json_object *j_dir = json_object_new_int(gpio_direction);

    json_object_object_add(jobj, "pin", j_pin);
    json_object_object_add(jobj, "direction", j_dir);

    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
}

/**
 * Pulse an output for a number of milliseconds.
 * @param cl the client who made the request.
 * @param request the request part of the url. 
 */
json_object* gpio_put_pulse_output(struct client *cl, char *request)
{
    /* This functions expects the following request /<gpiopin>/<mode>/<nr_of_ms> */
    int gpio_pin;
    int gpio_mode;
    int ms;

    /* If sscanf fails the request is malformed */
    if(sscanf(request, "%d/%d/%d", &gpio_pin, &gpio_mode, &ms) != 3) {
        cl->http_status = r_bad_req;
        return NULL;
    }

    if(!gpio_pulse(gpio_pin, ms*1000, gpio_mode)) {
        cl->http_status = r_error;
        return NULL;
    }

    /* Put data in JSON object */
    json_object *jobj = json_object_new_object();
    json_object *j_pin = json_object_new_int(gpio_pin);
    json_object *j_ms = json_object_new_int(ms);
    
    json_object_object_add(jobj, "pin", j_pin);
    json_object_object_add(jobj, "pulse_time", j_ms);

    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
}
