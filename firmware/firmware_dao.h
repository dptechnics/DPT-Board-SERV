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
 * File:   firmware_dao.h
 * Created on February 1, 2015, 4:02 PM
 */

#ifndef FIRMWARE_DAO_H
#define	FIRMWARE_DAO_H

#include "firmware.h"

/**
 * Call when database is set up. 
 * @return true on success, false on error.
 */
bool firmware_dao_init();

/**
 * Update the database with the latest firmware check result.
 * @param version the version of the remote available firmware.
 * @param release the release date of the remote available firmware.
 * @param url the url where the release can be downloaded from.
 * @param changes the changes as string array in the new version.
 * @param nrchanges the number of changes that are present.
 * @param firmware_path the local path of the firmware, if already downloaded.
 * @param newer true if the remote firmware version is newer than the one installed.
 * @param signature the md5 checksum of the file in the url.
 * @return true on success.
 */
bool firmware_dao_update_latest_firmware (int version, char* release, char* url, char** changes, size_t nrchanges, char* firmware_path, bool newer, char* signature);

/**
 * Get the latest firmware information from the database.
 * @param f_info the firmware info struct to fill.
 * @return true on success, false on error
 */
bool firmware_dao_get_latest_firmware(struct firmware_info *f_info);

#endif

