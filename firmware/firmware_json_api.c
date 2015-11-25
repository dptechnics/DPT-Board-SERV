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
 * File:   firmware_json_api.c
 * Created on February 1, 2015, 2:54 PM
 */

#include <json-c/json.h>

#include "../uhttpd.h"
#include "../logger.h"
#include "../helper.h"
#include "firmware_json_api.h"
#include "firmware.h"

/**
 * Route all get requests concerning the firmware module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function or NULL when the module is not found
 */
json_object* firmware_get_router(struct client *cl, char *request)
{
    if(helper_str_startswith(request, "check", 0))
    {
        return firmware_get_api_check(cl, request);
    } 
    else if (helper_str_startswith(request, "info", 0))
    {
        return firmware_get_api_info(cl, request);
    }
    else
    {
        log_message(LOG_WARNING, "Firmware API got unknown GET request '%s'\r\n", request);
        return NULL;
    }
}

/**
 * Route all post requests concerning the firmware module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* firmware_post_router(struct client *cl, char *request)
{
    if (helper_str_startswith(request, "download", 0)) 
    {
        return firmware_post_api_downloadupgrade(cl, request);
    } 
    else if (helper_str_startswith(request, "install", 0))
    {
        return firmware_post_api_apply(cl, request);
    }
    else
    {
        log_message(LOG_WARNING, "Firmware API got unknown POST request '%s'\r\n", request);
        return NULL;
    }
}

/**
 * Force the breakout-server to check for available firmware upgrades
 * @cl the client who made the request
 * @request the request part of the url
 * @return information about new firmware
 */
json_object* firmware_get_api_check(struct client *cl, char *request)
{
    struct firmware_info f_info;
    int i;
    
    /* Put data in JSON object */
    json_object *jobj = json_object_new_object();
    
    if(!firmware_check_upgrade(&f_info)) {
        log_message(LOG_ERROR, "Could not succesfully check new firmware version\r\n");
        json_object *j_error = json_object_new_string("Could not succesfully check new firmware version");
        json_object_object_add(jobj, "error", j_error);
        
        cl->http_status = r_error;
        return jobj;
    }
    
    json_object *j_status = json_object_new_boolean(!f_info.newer);
    json_object *j_version = json_object_new_int(f_info.version);
    json_object *j_release = json_object_new_string(f_info.release_date);
    json_object *j_url = json_object_new_string(f_info.url);
    
    json_object *j_changes = json_object_new_array();
    for(i = 0; i < f_info.changes_length; ++i) {
        json_object_array_add(j_changes, json_object_new_string(f_info.changes[i]));
    }
    
    json_object_object_add(jobj, "up-to-date", j_status);
    json_object_object_add(jobj, "version", j_version);
    json_object_object_add(jobj, "release-date", j_release);
    json_object_object_add(jobj, "url", j_url);
    json_object_object_add(jobj, "changes", j_changes);

    /* Free firmware information */
    firmware_free(&f_info);

    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
}

/**
 * Get the information about available upgrade stored in the database
 * @param cl the client who made the request
 * @param request the request part of the url
 * @return information about new firmware
 */
json_object* firmware_get_api_info(struct client *cl, char *request)
{
    json_object *jobj = json_object_new_object();
    struct firmware_info f_info;
    int i;
    
    if(!firmware_get_db_version(&f_info)) {
        log_message(LOG_ERROR, "Could not retreive information about firmware\r\n");
        cl->http_status = r_error;
        return NULL;
    }
    
    json_object *j_status = json_object_new_boolean(!f_info.newer);
    json_object *j_version = json_object_new_int(f_info.version);
    json_object *j_release = json_object_new_string(f_info.release_date);
    json_object *j_url = json_object_new_string(f_info.url);
    json_object *j_downloaded = json_object_new_boolean(f_info.downloaded);
    
    json_object *j_changes = json_object_new_array();
    for(i = 0; i < f_info.changes_length; ++i) {
        json_object_array_add(j_changes, json_object_new_string(f_info.changes[i]));
    }
    
    json_object_object_add(jobj, "up-to-date", j_status);
    json_object_object_add(jobj, "version", j_version);
    json_object_object_add(jobj, "release-date", j_release);
    json_object_object_add(jobj, "url", j_url);
    json_object_object_add(jobj, "changes", j_changes);
    json_object_object_add(jobj, "downloaded", j_downloaded);
    
    firmware_free(&f_info);
    cl->http_status = r_ok;
    return jobj;
}

/**
 * Download the firmware version, if newer, saved in the database
 * @cl the client who made the request.
 * @request the request part of the url.
 * @return true when download has started.
 */
json_object* firmware_post_api_downloadupgrade(struct client *cl, char *request)
{
    /* Put data in JSON object */
    json_object *jobj = json_object_new_object();
    json_object *j_status = json_object_new_boolean(firmware_latest_download());

    json_object_object_add(jobj, "status", j_status);

    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
}

/**
 * Apply downloaded firmware.
 * @param cl the client who made the request
 * @param request the request part of the url
 * @return json object marking success or not
 */
json_object* firmware_post_api_apply(struct client *cl, char *request)
{
    /* Parse JSON post data */
    json_object *in_obj = json_tokener_parse(cl->postdata);
    
    json_object *j_keep_settings = NULL;
    if(!json_object_object_get_ex(in_obj, "keep_settings", &j_keep_settings)) {
        if(json_object_put(in_obj) != 1) {
            log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
        }
        return NULL;
    }
    bool keep_settings = json_object_get_boolean(j_keep_settings);
    
    /* Create info object */
    json_object *jobj = json_object_new_object();
    bool firmware_status = firmware_apply_download(keep_settings);
    json_object *j_status = json_object_new_boolean(firmware_status);
    json_object_object_add(jobj, "status", j_status);


    /* Release parsed json object */
    if(json_object_put(in_obj) != 1) {
        log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
    }
    
    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;   
}
