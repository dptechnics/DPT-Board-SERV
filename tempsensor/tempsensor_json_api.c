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
 * File:   tempsensor_json_api.c
 * Created on February 9, 2015, 10:09 PM
 */

#include <json-c/json.h>

#include "../logger.h"
#include "../uhttpd.h"
#include "../helper.h"
#include "tempsensor.h"
#include "tempsensor_json_api.h"


/**
 * Route all get requests concerning the tempsensor module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* tempsensor_get_router(struct client *cl, char *request)
{
    if (helper_str_startswith(request, "read", 0)) 
    {
        return tempsensor_get_temperature(cl, request);
    } 
    else
    {
        log_message(LOG_WARNING, "Tempsensor API got unknown GET request '%s'\r\n", request);
        return NULL;
    }
}

/**
 * Route all post requests concerning the tempsensor module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* tempsensor_post_router(struct client *cl, char *request)
{
    return NULL;
}

/**
 * Get the temperature of a certain temperature sensor. 
 * @param cl the client who made the request
 * @param request teh request part of the url
 * @return the current temperature
 */
json_object* tempsensor_get_temperature(struct client *cl, char *request)
{
    /* Put data in JSON object */
    json_object *jobj = json_object_new_object();
    json_object *j_temp = json_object_new_double((double) tempsensor_current_temp(0));

    json_object_object_add(jobj, "temperature", j_temp);

    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
}
