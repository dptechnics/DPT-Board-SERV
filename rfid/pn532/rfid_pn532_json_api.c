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
 * File:   rfid_pn532_json_api.c
 * Created on December 8, 2015, 5:35 PM
 */

#include <json-c/json.h>
#include <stdint.h>
#include <stdbool.h>

#include "../../logger.h"
#include "../../uhttpd.h"
#include "../../helper.h"
#include "rfid_pn532.h"
#include "rfid_pn532_json_api.h"

/**
 * Route all get requests concerning the rfid_pn532 module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* rfid_pn532_get_router(struct client *cl, char *request)
{
    if (helper_str_startswith(request, "pn532/init", 0)) 
    {
        return rfid_pn532_json_get_init(cl, request);
    } else if(helper_str_startswith(request, "pn532/firmwareversion", 0)) {
        return rfid_pn532_json_get_firmware_version(cl, request);
    } else if(helper_str_startswith(request, "pn532/taguid", 0)) {
        return rfid_pn532_json_get_tag_uid(cl, request);
    }
    else
    {
        log_message(LOG_WARNING, "RFID PN532 API got unknown GET request '%s'\r\n", request);
        return NULL;
    }
}

/**
 * Route all post requests concerning the rfid_pn532 module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* rfid_pn532_post_router(struct client *cl, char *request)
{
    return NULL;
}

/**
 * Initialize a connected RFID reader.
 * @param cl the client who made the request
 * @param request teh request part of the url
 * @return true on success.
 */
json_object* rfid_pn532_json_get_init(struct client *cl, char *request)
{
    bool status = rfid_pn532_init_i2c(23,20,19, 6);
    
    /* Put data in JSON object */
    json_object *jobj = json_object_new_object();
    json_object *j_status = json_object_new_boolean(status);

    json_object_object_add(jobj, "status", j_status);

    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
    
}

/**
 * Get the firmware version of the connected RFID reader.
 * @param cl the client who made the request.
 * @param request teh request part of the url.
 * @return the current firmware version.
 */
json_object* rfid_pn532_json_get_firmware_version(struct client *cl, char *request)
{
    /* Put data in JSON object */
    json_object *jobj = json_object_new_object();
    json_object *j_fwv = json_object_new_int64((int64_t) rfid_pn532_get_firmware_version());

    json_object_object_add(jobj, "firmware_version", j_fwv);

    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
}

/**
 * Get a UID from a tag in the NFC field. 
 * @param cl the client who made the request.
 * @param request teh request part of the url.
 * @return the UID from the NFC tag if there is one. 
 */
json_object* rfid_pn532_json_get_tag_uid(struct client *cl, char *request)
{
    bool result = false;
    char uidstrbuf[16];
    uint32_t versiondata = rfid_pn532_get_firmware_version();
    
    uidstrbuf[0] = '\0';
    
    if(!versiondata) {
        log_message(LOG_ERROR, "Could not find PN53x board\r\n");
    } else {
        log_message(LOG_DEBUG, "Found chip PN5%02x, firmware version: %d.%d\r\n", 
                (versiondata>>24) & 0xFF, 
                (int) (versiondata>>16) & 0xFF,
                (int) (versiondata>>8) & 0xFF);

        // Set the max number of retry attempts to read from a card
        // This prevents us from waiting forever for a card, which is
        // the default behaviour of the PN532.
        //rfid_pn532_set_passive_activation_retries(0xFF);

        // Configure board to read RFID tags
        if(!rfid_pn532_config_SAM()) {
            log_message(LOG_ERROR, "Could not configure PN532 to read RFID tags\r\n");
        } else {
            uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };	// Buffer to store the returned UID
            uint8_t uid_len;				// Length of the UID (4 or 7 bytes depending on ISO14443A card type)

            result = rfid_pn532_read_passive_target_id(RFID_PN532_MIFARE_ISO14443A, &uid[0], &uid_len, 5000);

            if(result) {
                log_message(LOG_DEBUG, "Found an NFC card\r\n");
                if(uid_len == 4) {
                    sprintf(uidstrbuf, "%02x%02x%02x%02x", uid[3], uid[2], uid[1], uid[0]);
                } else {
                    sprintf(uidstrbuf, "%02x%02x%02x%02x%02x%02x%02x", uid[6], uid[5], uid[4], uid[3], uid[2], uid[1], uid[0]);
                }
                uidstrbuf[uid_len*2] = '\0';   
            } else {
                log_message(LOG_ERROR, "Could not read NFC tag or timeout\r\n");
            }
        }
    }
    
    /* Put data in JSON object */
    json_object *jobj = json_object_new_object();
    json_object *j_status = json_object_new_boolean(result);
    json_object *j_uid = json_object_new_string(uidstrbuf);

    json_object_object_add(jobj, "status", j_status);
    json_object_object_add(jobj, "result", j_uid);

    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
}

