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
 * File:   firmware.h
 * Created on January 21, 2015, 1:03 PM
 */

#ifndef FIRMWARE_H
#define	FIRMWARE_H

struct firmware_info {
    int version;            /* Firmware version */
    char release_date[11];  /* String containing the release date of this version */
    char *url;              /* URL pointing to the firmware */
    bool newer;             /* True when this version is newer than the one installed on the board */
    char** changes;         /* String array containing changes for this version */
    size_t changes_length;  /* The number of changes */
    char signature[33];     /* MD5 checksum of the firmware file */
    bool downloaded;        /* True if the firmware file exists on disk */
};

/**
 * Get the currently installed firmware version.
 * @return the firmware version on success, -1 on error.
 */
int firmware_get_installed_version();

/**
 * Check if new dpt-board upgrade is available.
 * @param struct f_info information about latest firmware version will be written here
 * @return true on success false on error.
 */
bool firmware_check_upgrade(struct firmware_info *f_info);

/**
 * Get the firmware update information currently stored in the the database
 * @param f_info information about the firmware in the database will be written here
 * @return true on success, false on error.
 */
bool firmware_get_db_version(struct firmware_info *f_info);

/**
 * Check if the firmware is downloaded and 
 * @param f_info information about the firmware. 
 * @return true if the downloaded firmware has the same signature. 
 */
bool firmware_is_downloaded();

/**
 * Download the latest firmware to RAM, returns false when already on the newest version
 * @return true on success false on error.
 */
bool firmware_latest_download();

/**
 * Install the downloaded software
 * @param keep_settings true when you want to keep settings after firmware upload
 * @return true on success false on error.
 */
bool firmware_apply_download(bool keep_settings);

/**
 * Free the firmware structure
 * @param struct f_info the firmware structure to free
 */
void firmware_free(struct firmware_info *f_info);
#endif

