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
 * File:   firmware_dao.c
 * Created on February 1, 2015, 4:02 PM
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "firmware_dao.h"
#include "../database/database.h"
#include "../database/db_keyvalue.h"
#include "../logger.h"
#include "../helper.h"

/* KeyValue keys */
#define DB_FIRMWARE_VERSION "firmware_version"
#define DB_FIRMWARE_URL "firmware_url"
#define DB_FIRMWARE_RELEASE "firmware_release"
#define DB_FIRMWARE_CHANGES "firmware_changes"
#define DB_FIRMWARE_NEWER "firmware_newer"
#define DB_FIRMWARE_FILEPATH "firmware_path"
#define DB_FIRMWARE_SIGNATURE "firmware_sig"

/**
 * Call when database is set up. 
 * @return true on success, false on error.
 */
bool firmware_dao_init()
{
    dao_keyvalue_put_int(DB_FIRMWARE_VERSION, 0);
    dao_keyvalue_put_text(DB_FIRMWARE_RELEASE, "");
    dao_keyvalue_put_text(DB_FIRMWARE_CHANGES, "");
    dao_keyvalue_put_text(DB_FIRMWARE_URL, "");
    dao_keyvalue_put_int(DB_FIRMWARE_NEWER, 0);
    dao_keyvalue_put_text(DB_FIRMWARE_FILEPATH, "");
    dao_keyvalue_put_text(DB_FIRMWARE_SIGNATURE, "");
    // TODO: error checking
    return true;
}

/**
 * Update the database with the latest firmware check result.
 * @param version the version of the remote available firmware.
 * @param release the release date of the remote available firmware.
 * @param url the url where the release can be downloaded from.
 * @param changes the changes as string array in the new version.
 * @param nrchanges the number of changes that are present.
 * @param firmware_path the local path of the firmware, if already downloaded.
 * @param newer true if the remote firmware version is newer than the one installed.
 * @return true on success.
 */
bool firmware_dao_update_latest_firmware (int version, char* release, char* url, char** changes, size_t nrchanges, char* firmware_path, bool newer, char* signature)
{    
    if(dao_keyvalue_edit_int(DB_FIRMWARE_VERSION, version) != DB_OK)
        return false;
    if(dao_keyvalue_edit_text(DB_FIRMWARE_RELEASE, release) != DB_OK)
        return false;
    if(dao_keyvalue_edit_text(DB_FIRMWARE_URL, url) != DB_OK)
        return false;
    if(dao_keyvalue_edit_text(DB_FIRMWARE_FILEPATH, firmware_path) != DB_OK)
        return false;
    if(dao_keyvalue_edit_int(DB_FIRMWARE_NEWER, newer ? 1 : 0) != DB_OK)
        return false;
    if(dao_keyvalue_edit_text(DB_FIRMWARE_SIGNATURE, signature) != DB_OK)
        return false;
    
    /* Serialize and save string array */
    char *ser_changes = helper_serialize_str_array(changes, nrchanges);
    if(dao_keyvalue_edit_text(DB_FIRMWARE_CHANGES, ser_changes) != DB_OK){
        free(ser_changes);
        return false;
    }
    
    /* Clean up the serialized string */
    free(ser_changes);
    
    return true;
}

/**
 * Get the latest firmware information from the database.
 * @param f_info the firmware info struct to fill.
 * @return true on success, false on error
 */
bool firmware_dao_get_latest_firmware(struct firmware_info *f_info)
{
    db_int *i;
    db_text *t;
    
    i = dao_keyvalue_get_int(DB_FIRMWARE_VERSION);
    if(i->status != DB_OK) {
        log_message(LOG_ERROR, "Could not read remote firmware version from database\r\n");
        dao_destroy_db_int(i);
        return false;
    }
    f_info->version = i->value;
    dao_destroy_db_int(i);
    
    t = dao_keyvalue_get_text(DB_FIRMWARE_RELEASE);
    if(t->status != DB_OK) {
        log_message(LOG_ERROR, "Could not read remote firmware release date from database\r\n");
        dao_destroy_db_text(t);
        return false;
    }
    strcpy(f_info->release_date, t->value);
    dao_destroy_db_text(t);
    
    t = dao_keyvalue_get_text(DB_FIRMWARE_SIGNATURE);
    if(t->status != DB_OK) {
        log_message(LOG_ERROR, "Could not read remote firmware signature from database\r\n");
        dao_destroy_db_text(t);
        return false;
    }
    strncpy(f_info->signature, t->value, 32);
    f_info->signature[32] = '\0';
    dao_destroy_db_text(t);
    
    t = dao_keyvalue_get_text(DB_FIRMWARE_URL);
    if(t->status != DB_OK) {
        log_message(LOG_ERROR, "Could not read remote firmware url from database\r\n");
        dao_destroy_db_text(t);
        return false;
    }
    f_info->url = t->value;
    free(t);
    
    i = dao_keyvalue_get_int(DB_FIRMWARE_NEWER);
    if(i->status != DB_OK) {
        log_message(LOG_ERROR, "Could not read remote firmware 'newer' status from database\r\n");
        dao_destroy_db_int(i);
        return false;
    }
    f_info->newer = i->value == 1;
    dao_destroy_db_int(i);
    
    t = dao_keyvalue_get_text(DB_FIRMWARE_CHANGES);
    if(t->status != DB_OK) {
        log_message(LOG_ERROR, "Could not read remote firmware changes from database\r\n");
        dao_destroy_db_text(t);
        return false;
    }
    struct chararray* chr_array = helper_unserialize_str_array(t->value);
    f_info->changes = chr_array->array;
    f_info->changes_length = chr_array->len;
    free(chr_array);
    dao_destroy_db_text(t);
    
    return true;
}
