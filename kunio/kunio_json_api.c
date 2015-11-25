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
 * File:   kunio_json_api.c
 * Created on April 6, 2015, 5:17 PM
 */

#include <stdio.h>
#include <stdio.h>
#include <json-c/json.h>
#include <string.h>
#include <stdlib.h>

#include "../helper.h"
#include "../logger.h"
#include "kunio.h"
#include "kunio_json_api.h"


/**
 * Route all get requests concerning the kunio module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* kunio_get_router(struct client *cl, char *request)
{
    if (helper_str_startswith(request, "state", 0)) 
    {
        return kunio_get_state(cl, request);
    } 
    else
    {
        log_message(LOG_WARNING, "KunIO API got unknown GET request '%s'\r\n", request);
        return NULL;
    }
}

/**
 * Route all put requests concerning the kunio module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* kunio_put_router(struct client *cl, char *request)
{
    if (helper_str_startswith(request, "state", 0)) 
    {
        return kunio_put_state(cl, request);
    } 
    else if (helper_str_startswith(request, "enable", 0)) 
    {
        return kunio_put_enable(cl, request);
    } 
    else
    {
        log_message(LOG_WARNING, "KunIO API got unknown PUT request '%s'\r\n", request);
        return NULL;
    }
}

/**
 * Get the state of the alfaio input module.
 * @cl the client who made the request
 * @request the request part of the url
 */
json_object* kunio_get_state(struct client *cl, char *request)
{
    alfa_read_input();

    /* Put data in JSON object */
    json_object *jobj = json_object_new_object();
    json_object *j_pin = json_object_new_int(1);
    json_object *j_state = json_object_new_int(1);

    json_object_object_add(jobj, "pin", j_pin);
    json_object_object_add(jobj, "state", j_state);

    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
}

/**
 * Control the ports of the AlfaIO output module
 */
json_object* kunio_put_state(struct client *cl, char *request)
{
	uint8_t tx[] = {0xAA};
	alfa_set_output(tx, 1);

	/* Put data in JSON object */
	json_object *jobj = json_object_new_object();
	json_object *j_pin = json_object_new_int(1);
	json_object *j_state = json_object_new_int(1);

	json_object_object_add(jobj, "pin", j_pin);
	json_object_object_add(jobj, "state", j_state);

	/* Return status ok */
	cl->http_status = r_ok;
	return jobj;
}

/**
 * Control the ports of the KunIO output module
 */
json_object* kunio_put_enable(struct client *cl, char *request)
{
	/* This functions expects the following request /<enable> */
	int enable;

	/* If sscanf fails the request is malformed */
	if(sscanf(request, "%d", &enable) != 1) {
		cl->http_status = r_bad_req;
		return NULL;
	}

	alfa_set_enable(enable == 1);

	/* Put data in JSON object */
	json_object *jobj = json_object_new_object();
	json_object *j_state = json_object_new_boolean(enable == 1);

	json_object_object_add(jobj, "state", j_state);

	/* Return status ok */
	cl->http_status = r_ok;
	return jobj;
}
