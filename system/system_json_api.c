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
 * File:   system_json_api.c
 * Created on April 6, 2015, 5:28 PM
 */


#include <json-c/json.h>
#include "../logger.h"
#include "../uhttpd.h"
#include "../helper.h"

#include "system.h"
#include "system_json_api.h"
#include "../wifi/wifi.h"

/**
 * Route all get requests concerning the system module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* system_get_router(struct client *cl, char *request)
{
    if (helper_str_startswith(request, "diskspace", 0)) 
    {
        return system_get_free_disk_space(cl, request);
    } 
    else if (helper_str_startswith(request, "overview", 0))
    {
        return system_get_overview(cl, request);
    }
    else
    {
        log_message(LOG_WARNING, "System API got unknown GET request '%s'\r\n", request);
        return NULL;
    }
}

/**
 * Get free disk space if a mounted filesystem
 * could be found.
 * @cl the client who made the request
 * @request the request part of the url
 */
json_object* system_get_free_disk_space(struct client *cl, char *request)
{
    /* Create info object */
    json_object *jobj = json_object_new_object();
    json_object *freespace = json_object_new_int(system_get_usb_storage_freespace());
    json_object *totalspace = json_object_new_int(system_get_usb_storage_totalspace());

    json_object_object_add(jobj, "free", freespace);
    json_object_object_add(jobj, "total", totalspace);

    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
}

/**
 * Get the state of the overall board.
 * @cl the client who made the request
 * @request the request part of the url
 */
json_object* system_get_overview(struct client *cl, char *request)
{
    char *hostname;
    char *model;
    char *load;

    if( (hostname = system_get_hostname()) == NULL ||		/* Get the system hostname */
        (model = system_get_model()) == NULL ||			/* Get the system model */
        (load = system_get_system_load()) == NULL)		/* Get the current system load */
    {
        cl->http_status = r_error;
        return NULL;
    }


    /* Put data in JSON object */
    json_object *jobj = json_object_new_object();

    /* Create the JSON objects */
    json_object *j_hostname = json_object_new_string(hostname);
    json_object *j_model = json_object_new_string(model);
    json_object *j_eth0_state = json_object_new_boolean(system_is_eth_connected(0));
    json_object *j_eth1_state = json_object_new_boolean(system_is_eth_connected(1));
    json_object *j_usb_state = json_object_new_string(system_get_usb_storage_connection_state());
    json_object *j_usb_free = json_object_new_int(system_get_usb_storage_freespace());
    json_object *j_usb_total = json_object_new_int(system_get_usb_storage_totalspace());
    json_object *j_ram_free = json_object_new_int(system_get_ram_freespace());
    json_object *j_ram_total = json_object_new_int(system_get_ram_totalspace());
    json_object *j_system_load = json_object_new_string(load);


    /* Put data in JSON object */
    json_object_object_add(jobj, "sysname", j_hostname);
    json_object_object_add(jobj, "model", j_model);
    json_object_object_add(jobj, "eth0_connected", j_eth0_state);
    json_object_object_add(jobj, "eth1_connected", j_eth1_state);
    json_object_object_add(jobj, "usb_state", j_usb_state);
    json_object_object_add(jobj, "usb_free", j_usb_free);
    json_object_object_add(jobj, "usb_total", j_usb_total);

    json_object_object_add(jobj, "ram_free", j_ram_free);
    json_object_object_add(jobj, "ram_total", j_ram_total);
    json_object_object_add(jobj, "system_load", j_system_load);

    /* Free memory */
    free(hostname);
    free(model);
    free(load);

    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
}
