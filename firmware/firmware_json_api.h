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
 * File:   firmware_json_api.h
 * Created on February 1, 2015, 2:54 PM
 */

#ifndef FIRMWARE_JSON_API_H
#define	FIRMWARE_JSON_API_H

#include <json-c/json.h>
#include "../uhttpd.h"

/**
 * Route all get requests concerning the firmware module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* firmware_get_router(struct client *cl, char *request);

/**
 * Route all post requests concerning the firmware module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* firmware_post_router(struct client *cl, char *request);

/**
 * Force the breakout-server to check for available firmware upgrades
 * @cl the client who made the request
 * @request the request part of the url
 * @return information about new firmware
 */
json_object* firmware_get_api_check(struct client *cl, char *request);

/**
 * Get the information about available upgrade stored in the database
 * @param cl the client who made the request
 * @param request the request part of the url
 * @return information about new firmware
 */
json_object* firmware_get_api_info(struct client *cl, char *request);

/**
 * Download the firmware version, if newer, saved in the database
 * @cl the client who made the request.
 * @request the request part of the url.
 * @return true when download has started.
 */
json_object* firmware_post_api_downloadupgrade(struct client *cl, char *request);

/**
 * Apply downloaded firmware.
 * @param cl the client who made the request
 * @param request the request part of the url
 * @return json object marking success or not
 */
json_object* firmware_post_api_apply(struct client *cl, char *request);

#endif

