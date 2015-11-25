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
 * File:   wifi_json_api.h
 * Created on March 14, 2015, 10:04 PM
 */

#ifndef WIFI_JSON_API_H
#define	WIFI_JSON_API_H

#define WIFI_JSON_BOARD_NETWORK     1
#define WIFI_JSON_CLIENT_NETWORK    2

#define WIFI_ERROR_UNKNOWN_ERROR            100
#define WIFI_ERROR_BAD_NETWORK_DESCRIPTOR   101
#define WIFI_ERROR_SSID_TO_LONG             102
#define WIFI_ERROR_NO_RESTART               103
#define WIFI_RESULT_OK                      200

#include <json-c/json.h>
#include "../uhttpd.h"

/**
 * Route all get requests concerning the wifi module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* wifi_get_router(struct client *cl, char *request);

/**
 * Route all post requests concerning the wifi module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* wifi_post_router(struct client *cl, char *request);

/**
 * Scan for available wifi networks and return information 
 * @cl the client who made the request.
 * @request the request part of the url.
 * @return the wifi information
 */
json_object* wifi_get_scan(struct client *cl, char *request);

/**
 * Request a WiFi scan, this will go async, and return immediately. 
 * @param cl the client who made the request. 
 * @param request the request part of the url.
 * @return true on success, false on error.
 */
json_object* wifi_get_scantrigger(struct client *cl, char *request);

/**
 * Get all information about the wireless interfaces
 * currently active. 
 * @param cl the client who made the request. 
 * @param request the request part of the url. 
 * @return the wifi information
 */
json_object* wifi_get_info(struct client *cl, char *request);

/**
 * Post a new WiFi client configuration.
 * @cl the client who made the request
 * @request the request part of the url
 */
json_object* wifi_post_connect(struct client *cl, char *request);

/**
 * Change the ssid of a given network.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return json object marking success or not.
 */
json_object* wifi_post_ssid_change(struct client *cl, char *request);

/**
 * Change the state of a network.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return json object marking success or not.
 */
json_object* wifi_post_state_change(struct client *cl, char *request);

/**
 * Change the state and ssid of a network.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return json object marking success or not.
 */
json_object* wifi_post_simplesettings_change(struct client *cl, char *request);

#endif

