/* 
 * Copyright (c) 2015, Daan Pape
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
 * File:   rfid_pn532_json_api.h
 * Created on December 8, 2015, 5:45 PM
 */

#ifndef RFID_PN532_JSON_API_H
#define	RFID_PN532_JSON_API_H

#include <json-c/json.h>
#include "../../uhttpd.h"

/**
 * Route all get requests concerning the rfid_pn532 module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* rfid_pn532_get_router(struct client *cl, char *request);

/**
 * Route all post requests concerning the rfid_pn532 module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* rfid_pn532_post_router(struct client *cl, char *request);

/**
 * Initialize a connected RFID reader.
 * @param cl the client who made the request
 * @param request teh request part of the url
 * @return true on success.
 */
json_object* rfid_pn532_json_get_init(struct client *cl, char *request);

/**
 * Get the firmware version of the connected RFID reader.
 * @param cl the client who made the request.
 * @param request teh request part of the url.
 * @return the current firmware version.
 */
json_object* rfid_pn532_json_get_firmware_version(struct client *cl, char *request);

/**
 * Get a UID from a tag in the NFC field. 
 * @param cl the client who made the request.
 * @param request teh request part of the url.
 * @return the UID from the NFC tag if there is one. 
 */
json_object* rfid_pn532_json_get_tag_uid(struct client *cl, char *request);

#endif

