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
 * File:   bluecherry_json_api.c
 * Created on April 9, 2015, 9:00 AM
 */

#include <json-c/json.h>

#include "../uhttpd.h"
#include "../logger.h"
#include "../helper.h"
#include "bluecherry.h"
#include "bluecherry_json_api.h"

/**
 * Route all get requests concerning the bluecherry module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function or NULL when the module is not found
 */
json_object* bluecherry_get_router(struct client *cl, char *request) {
    if(helper_str_startswith(request, "status", 0)){
        return bluecherry_get_current_status(cl, request);
    }
    else {
        log_message(LOG_WARNING, "BlueCherry API got unknown GET request '%s'\r\n", request);
        return NULL;
    }
}

/**
 * Route all post requests concerning the bluecherry module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* bluecherry_post_router(struct client *cl, char *request) {
    if (helper_str_startswith(request, "login", 0)) {
        return bluecherry_post_login_user(cl, request);
    }
    else if (helper_str_startswith(request, "init", 0)) {
        return bluecherry_post_init_device(cl, request);
    }
    else {
        log_message(LOG_WARNING, "BlueCherry API got unknown POST request '%s'\r\n", request);
        return NULL;
    }
}

/**
 * Login the user into the BlueCherry platform. 
 * @param cl the client who made the request.
 * @param request the request part of the url. 
 * @return the result of the called function. 
 */
json_object* bluecherry_post_login_user(struct client *cl, char *request) {
    /* Parse JSON post data */
    json_object *in_obj = json_tokener_parse(cl->postdata);
    
    json_object *j_username = NULL;
    json_object *j_password = NULL;
    if(!json_object_object_get_ex(in_obj, "username", &j_username) ||
       !json_object_object_get_ex(in_obj, "password", &j_password)) 
    {
        if(json_object_put(in_obj) != 1) {
            log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
        }
        return NULL;
    }
    
    const char* username = json_object_get_string(j_username);
    const char* password = json_object_get_string(j_password);
    
    /* Create info object */
    json_object *jobj = json_object_new_object();
    json_object *j_status = json_object_new_boolean(bluecherry_login(username, password));
    json_object_object_add(jobj, "result", j_status);


    /* Release parsed json object */
    if(json_object_put(in_obj) != 1) {
        log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
    }
    
    /* Return status ok */
    cl->http_status = r_ok;
    return jobj; 
}

/**
 * Initialize device with BlueCherry
 * @param cl the client who made the request.
 * @param request the request part of the url. 
 * @return the result of the called function. 
 */
json_object* bluecherry_post_init_device(struct client *cl, char *request) {
    /* Parse JSON post data */
    json_object *in_obj = json_tokener_parse(cl->postdata);
    
    json_object *j_username = NULL;
    json_object *j_password = NULL;
    if(!json_object_object_get_ex(in_obj, "username", &j_username) ||
       !json_object_object_get_ex(in_obj, "password", &j_password)) 
    {
        if(json_object_put(in_obj) != 1) {
            log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
        }
        return NULL;
    }
    
    const char* username = json_object_get_string(j_username);
    const char* password = json_object_get_string(j_password);
    
    /* Create info object */
    json_object *jobj = json_object_new_object();
    json_object *j_status = json_object_new_int(bluecherry_device_init(username, password));
    json_object_object_add(jobj, "result", j_status);

    /* Release parsed json object */
    if(json_object_put(in_obj) != 1) {
        log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
    }
    
    /* Return status ok */
    cl->http_status = r_ok;
    return jobj; 
}

/**
 * Get the current BlueCherry status
 * @param cl the client who made the request.
 * @param request the request part of the url. 
 * @return the result of the current bluecherry status. 
 */
json_object* bluecherry_get_current_status(struct client *cl, char *request)
{    
    bluecherry_state state = bluecherry_status();
    
    /* Create info object */
    json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "status", json_object_new_int(state));
    
    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;     
}
