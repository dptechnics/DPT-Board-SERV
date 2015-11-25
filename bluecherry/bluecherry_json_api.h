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
 * File:   bluecherry_json_api.h
 * Created on April 9, 2015, 8:59 AM
 */



#ifndef BLUECHERRY_JSON_API_H
#define	BLUECHERRY_JSON_API_H

#include <json-c/json.h>

/**
 * Route all get requests concerning the bluecherry module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function or NULL when the module is not found
 */
json_object* bluecherry_get_router(struct client *cl, char *request);

/**
 * Route all post requests concerning the bluecherry module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* bluecherry_post_router(struct client *cl, char *request);

/**
 * Login the user into the BlueCherry platform. 
 * @param cl the client who made the request.
 * @param request the request part of the url. 
 * @return the result of the called function. 
 */
json_object* bluecherry_post_login_user(struct client *cl, char *request);

/**
 * Initialize device with BlueCherry
 * @param cl the client who made the request.
 * @param request the request part of the url. 
 * @return the result of the called function. 
 */
json_object* bluecherry_post_init_device(struct client *cl, char *request);

/**
 * Get the current BlueCherry status
 * @param cl the client who made the request.
 * @param request the request part of the url. 
 * @return the result of the current bluecherry status. 
 */
json_object* bluecherry_get_current_status(struct client *cl, char *request);

#endif

