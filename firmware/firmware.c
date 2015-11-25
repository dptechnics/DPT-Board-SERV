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
 * File:   firmware.c
 * Created on January 24, 2015, 5:06 PM
 */

#include <stdbool.h>
#include <curl/curl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include "firmware.h"
#include "firmware_dao.h"
#include "../config.h"
#include "../logger.h"
#include "../filedownload.h"

/**
 * True when the firmware is download onto the device
 */
bool downloaded;

/**
 * Struct containing download memory
 */
struct mem_string {
    char *str;
    size_t size;
};

/**
 * Callback function used by curl to support chunk downloading to memory
 */
static size_t mem_string_read_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct mem_string *mem = (struct mem_string*) userp;

    mem->str = realloc(mem->str, mem->size + realsize + 1);
    if(mem->str == NULL) {
        log_message(LOG_ERROR, "Not enough memory for download\r\n");
    }
    
    memcpy(&(mem->str[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->str[mem->size] = 0;
 
    return realsize;
}

/**
 * Get the currently installed firmware version.
 * @return the firmware version on success, -1 on error.
 */
int firmware_get_installed_version()
{
    FILE *fd = NULL;
    int local_version;
    
    fd = fopen(LOCAL_FIRMWARE_FILE, "r");  
    if(fd == NULL) {
        log_message(LOG_ERROR, "Could not open file '%s' containing current firmware version\r\n", LOCAL_FIRMWARE_FILE);
        return -1;
    }
    if(fscanf(fd, "%d", &local_version) != 1) {
        log_message(LOG_ERROR, "Unexpected format for local firmware version file '%s'\r\n", LOCAL_FIRMWARE_FILE);
        fclose(fd);
        return -1;
    }
    
    fclose(fd);
    return local_version;
}

/**
 * Check if new dpt-board upgrade is available.
 * @param struct f_info information about latest firmware version will be written here
 * @return true on success false on error.
 */
bool firmware_check_upgrade(struct firmware_info *f_info)
{
    CURL *curl;
    CURLcode res;
    struct mem_string mem;
    json_object *in_obj = NULL;
    int local_version, i;
    
    /* Allocate memory */
    mem.str = malloc(1);
    mem.size = 0;
    
    curl = curl_easy_init();
    if(!curl) {
        log_message(LOG_ERROR, "Could not instantiate CURL to check firmware version\r\n");
        return false;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, conf->public_firmware_uri);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mem_string_read_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &mem);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, CURL_USER_AGENT);

    res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        log_message(LOG_ERROR, "Curl download failed: %s\r\n", curl_easy_strerror(res));
        goto error;
    }
    
    /* Read latest available firmware version */
    mem.str[mem.size-1] = '\0';
    in_obj = json_tokener_parse(mem.str);
    if(in_obj == NULL) {
        goto error;
    }
    
    json_object *latest = NULL;
    json_object *j_version = NULL;
    json_object *j_rel_date = NULL;
    json_object *j_url = NULL;
    json_object *j_changes = NULL;
    json_object *j_md5 = NULL;
    
    if(!json_object_object_get_ex(in_obj, "latest", &latest) ||
       !json_object_object_get_ex(latest, "version", &j_version) ||
       !json_object_object_get_ex(latest, "release-date", &j_rel_date) ||
       !json_object_object_get_ex(latest, "url", &j_url) ||
       !json_object_object_get_ex(latest, "changes", &j_changes) ||
       !json_object_object_get_ex(latest, "md5sum", &j_md5)) {
        goto error;
    }
    
    f_info->version = json_object_get_int(j_version);
    
    const char *release_date = json_object_get_string(j_rel_date);
    strcpy(f_info->release_date, release_date);
    
    const char *url = json_object_get_string(j_url);
    f_info->url = (char*) malloc((strlen(url) + 1) * sizeof(char));
    strcpy(f_info->url, url);
    
    int length = json_object_array_length(j_changes);
    f_info->changes = (char**) malloc(length*sizeof(char*));
    for(i = 0; i < length; ++i) {
        const char *str = json_object_get_string(json_object_array_get_idx(j_changes, i));
        f_info->changes[i] = (char*) malloc((strlen(str)+1)*sizeof(char));
        strcpy(f_info->changes[i], str);
    }
    f_info->changes_length = length;
    
    const char *md5_sum = json_object_get_string(j_md5);
    strncpy(f_info->signature, md5_sum, 32);
    f_info->signature[32] = '\0';
    
    /* Read currently installed firmware version */
    local_version = firmware_get_installed_version();
    if(local_version < 0) {
        log_message(LOG_ERROR, "Could not determine local firmware version\r\n");
        goto error;
    }
    
    /* Check if remote firmware is newer or not */
    f_info->newer = f_info->version > local_version;
    
    /* Save this check in the database */
    if(!firmware_dao_update_latest_firmware(
        f_info->version,
        f_info->release_date,
        f_info->url,
        f_info->changes,
        f_info->changes_length,
        "",
        f_info->newer,
        f_info->signature
    )) {
        log_message(LOG_ERROR, "Could not save firmware update information in database\r\n");
        goto error;
    }
    
    /* Cleanup */
    curl_easy_cleanup(curl);
    if(mem.str) {
        free(mem.str);
    }
    if(json_object_put(in_obj) != 1) {
        log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
    }
    return true;

error:
    curl_easy_cleanup(curl);
    if(mem.str) {
        free(mem.str);
    }
    if(in_obj != NULL) {
        if(json_object_put(in_obj) != 1) {
            log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
        }
    }
    return false;
}

/**
 * Get the firmware update information currently stored in the the database
 * @param f_info information about the firmware in the database will be written here
 * @return true on success, false on error.
 */
bool firmware_get_db_version(struct firmware_info *f_info)
{  
    /* Get firmware information from database */
    if(!firmware_dao_get_latest_firmware(f_info)) {
        log_message(LOG_ERROR, "Firmware download could not read firmware info from database\r\n");
        return false;
    }
    
    /* Add dynamic firmware downloaded check */
    f_info->downloaded = firmware_is_downloaded();
    return true;          
}

/**
 * Check if the firmware is downloaded and 
 * @param f_info information about the firmware. 
 * @return true if the downloaded firmware has the same signature. 
 */
bool firmware_is_downloaded()
{    
    return downloaded && access(FIRMWARE_FILE_PATH, F_OK) != -1;
}

/**
 * Callback when download is complete, this is called in the 
 * download thread. 
 * @param d_info struct containing information about the download
 * @return NULL
 */
static void* firmware_download_callback(struct download_info *d_info)
{
    FILE *sys;
    char md5sum[33];
    
    log_message(LOG_INFO, "File downloaded: %d\r\n", d_info->status);
    free(d_info);
    
    /* Check download md5 sum */
    struct firmware_info f_info;
    if(!firmware_dao_get_latest_firmware(&f_info)) {
        log_message(LOG_ERROR, "Firmware download could not read firmware info from database\r\n");
        return NULL;
    }
    
    /* Get md5 sum of file */
    sys = popen("md5sum " FIRMWARE_FILE_PATH "", "r");
    if(sys == NULL) {
        downloaded = false;
        return NULL;
    }
    
    /* Read the first 32 characters from the output */
    fgets(md5sum, 33, sys);
    pclose(sys);
    
    /* Check if the firmware downloaded successfully */
    log_message(LOG_DEBUG, "MD5 signature of downloaded file: %s, MD5 of remote firmware: %s\r\n", md5sum, f_info.signature);
    if(strcmp(md5sum, f_info.signature) == 0) {
        downloaded = true;
    } else {
        downloaded = false;
    }
    
    firmware_free(&f_info);

    return NULL;
}

/**
 * Download the latest firmware to RAM, returns false when already on the newest version
 * @return true on success false on error.
 */
bool firmware_latest_download()
{
    bool result;
    
    /* Reset download status */
    downloaded =  false;
    
    /* Download latest firmware information */
    struct firmware_info f_info;
    struct download_info *d_info = calloc(1, sizeof(struct download_info));
    
    if(!firmware_dao_get_latest_firmware(&f_info)) {
        log_message(LOG_ERROR, "Firmware download could not read firmware info from database\r\n");
        return false;
    }
    
    /* Try to download the url */
    result = filedownload_async_download(f_info.url, FIRMWARE_FILE_PATH, FIRMWARE_FILE_NAME, d_info, firmware_download_callback);
    
    firmware_free(&f_info);
    return result;
}

/**
 * Entry point for firmware sysupgrade, this will reboot the system completely 
 * and flash new firmware to rom. 
 * @param keep_settings
 */
static void* _firmware_apply_thread(void *arg)
{
    bool keep_settings = ((char*) arg)[0] == 't';
    
    if(keep_settings) {
        log_message(LOG_INFO, "Breakout server is upgrading system while saving settings\r\n");
        system("sysupgrade " FIRMWARE_FILE_PATH);
    } else {
        log_message(LOG_INFO, "Breakout server is upgrading system to factory defaults\r\n");
        system("sysupgrade -n " FIRMWARE_FILE_PATH);
    }
    
    pthread_exit(NULL);
}

/**
 * Install the downloaded software
 * @param keep_settings
 * @return true on success false on error.
 */
bool firmware_apply_download(bool keep_settings)
{
    pthread_t u_thread;
    char* settings = (char*) malloc(1*sizeof(char));
    settings[0] = keep_settings ? 't' : 'f';
    
    
    /* Check if firmware file is present on disk */
    if(access(FIRMWARE_FILE_PATH, F_OK) == -1) {
        log_message(LOG_WARNING, "Firmware apply could not find firmware file: '%s'\r\n");
        return false;
    }
    
    if(pthread_create(&u_thread, NULL, _firmware_apply_thread, (void*) settings) != 0) {
        log_message(LOG_ERROR, "Could not start firmware apply thread\r\n");
        return false;
    }
    pthread_detach(u_thread);
    return true;
}

/**
 * Free the firmware structure
 * @param the firmware structure to free
 */
void firmware_free(struct firmware_info *f_info)
{
    int i;
    
    if(f_info->url != NULL) {
        free(f_info->url);
    }
    if(f_info->changes_length > 0){
        for(i = 0; i < f_info->changes_length; ++i){
            free(f_info->changes[i]);
        }
        free(f_info->changes);
    }
}
