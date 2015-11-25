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
 * File:   config.c
 * Created on March 14, 2015, 11:29 PM
 */

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include "config.h"
#include "logger.h"

/**
 * Trim leading and trailing whitespace from a function. Original
 * by Adam Rosenfield on http://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
 * @param str the string to trim.
 * @return the trimmed string. Do not call free on this one. 
 */
static char* trimwhitespace(char *str) {
    char *end;

    // Trim leading space
    while (isspace(*str)) str++;

    if (*str == 0)
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;

    // Write new null terminator
    *(end + 1) = 0;

    return str;
}

/**
 * Reserve memory and copy a string to it, be assured the string
 * is 0-terminated. 
 * @param destination the destination to copy to, if NULL new memory will be allocated. 
 * @param source the source string to copy. 
 * @return NULL on 
 */
static char* strmalloc(char* destination, const char* source) {
    size_t s_size = strlen(source);
    
    char* buff = (char*) realloc(destination, (s_size + 1) * sizeof(char));
    
    if(buff == NULL) {
        // Could not allocate memory, out of memory?
        log_message(LOG_ERROR, "Could not allocate memory in config.c:63, out of memory?\r\n");
        return NULL;
    }
    
    memcpy(buff, source, s_size);
    
    // Make sure string is null terminated
    *(buff+s_size) = '\0';
    
    return buff;
}

/**
 * Parse an integer from a string on a safe way. 
 * @param string the string to parse to integer.
 * @param unsignedint if set to true, the fallback value will be used if the integer is negative.  
 * @param integer to fallback to if parsing fails.
 * @return the parsed integer on success, fallback on failure. 
 */
static int parseint(char* string, bool unsignedint, int fallback) {
    errno = 0;
    char* end = string;
    int result =  strtol(string, &end, 10);
    
    if (errno == ERANGE) {
        log_message(LOG_WARNING, "Configuration parameter 'buff_size' is to large  (overflow), falling back to default\r\n");
        return fallback;
    }
    
    if(unsignedint) {
        /* Parse as unsigned integer */
        if ((!isdigit(string) || *string=='+') || *end) {
            log_message(LOG_WARNING, "Configuration parameter 'buff_size' is not a valid integer, falling back to default\r\n");
            return fallback;
        }
    } else {
        /* Parse as signed integer */
        if(*end) {
            log_message(LOG_WARNING, "Configuration parameter 'buff_size' is not a valid integer, falling back to default\r\n");
            return fallback;
        }
    }
    
    return result;
}

config* conf = NULL;

/**
 * Parse configuration file at /etc/config/dpt-connector
 * @return true when parse was successfull
 */
bool config_parse() {
    /* Read buffer */
    char buffer[512];
    FILE * fd;
    char* key;
    char* value;
    
    /* Reserve memory for configuration and null terminate every string*/
    conf = (config*) malloc(sizeof(config));
    
    /* Put in default configuration */
    conf->daemon = FORK_ON_START;
    
    conf->listen_port = strmalloc(NULL, LISTEN_PORT);
    conf->database = strmalloc(NULL, DB_LOCATION);
    conf->keep_alive_time = KEEP_ALIVE_TIME;
    conf->network_timeout = NETWORK_TIMEOUT;
    
    conf->index_file = strmalloc(NULL, INDEX_FILE);
    conf->document_root = strmalloc(NULL, DOCUMENT_ROOT);
    conf->api_prefix = strmalloc(NULL, API_PATH);
    conf->api_str_len = strlen(API_PATH) + 1;
    
    conf->public_firmware_uri = strmalloc(NULL, PUBLIC_FIRMWARE_FILE);
    conf->firmware_download_path = strmalloc(NULL, FIRMWARE_FILE_PATH);
    conf->firmware_file_name = strmalloc(NULL, FIRMWARE_FILE_NAME);
    
    conf->ubus_timeout = UBUS_TIMEOUT;
    
    
    /* Parse configuration file if any */
    if ((fd = fopen(CONFIGURATION_FILE, "r")) != NULL) {

        while (fgets(buffer, CONFIG_BUFF_SIZE, fd) != NULL) {
            /* Ignore lines starting with '#', ';' or whitespace  */
            if (buffer[0] != '#' && buffer[0] != ';' && buffer[0] != ' ' && buffer[0] != '\t' && buffer[0] != '\r' && buffer[0] != '\n') {
                char* trimmed = trimwhitespace(buffer);
                key = strtok(trimmed, " \t");
                value = strtok (NULL, " ,.-");
                
                if(strcmp(key, "daemon") == 0) 
                {
                    conf->daemon = value[0] == 't';
                } 
                else if (strcmp(key, "listen_port") == 0) 
                { 
                    conf->listen_port = strmalloc(conf->listen_port, value);
                } 
                else if (strcmp(key, "database") == 0)
                {
                    conf->database = strmalloc(conf->database, value);
                }
                else if (strcmp(key, "keep_alive_time") == 0) 
                {
                    conf->keep_alive_time = parseint(value, true, KEEP_ALIVE_TIME);
                }
                else if (strcmp(key, "network_timeout") == 0) 
                {
                    conf->network_timeout = parseint(value, true, NETWORK_TIMEOUT);
                }
                else if (strcmp(key, "index_file") == 0) 
                {
                    conf->index_file = strmalloc(conf->index_file, value);
                }
                else if (strcmp(key, "document_root") == 0) 
                {
                    conf->document_root = strmalloc(conf->document_root, value);
                }
                else if (strcmp(key, "api_prefix") == 0) 
                {
                    conf->api_prefix = strmalloc(conf->api_prefix, value);
                    conf->api_str_len = strlen(value) + 1;
                }
                else if (strcmp(key, "public_firmmware_uri") == 0) 
                {
                    conf->public_firmware_uri = strmalloc(conf->public_firmware_uri, value);
                }
                else if (strcmp(key, "firmware_download_path") == 0) 
                {
                    conf->firmware_download_path = strmalloc(conf->firmware_download_path, value);
                }
                else if (strcmp(key, "firmware_file_name") == 0) 
                {
                    conf->firmware_file_name = strmalloc(conf->firmware_file_name, value);
                }
                else if (strcmp(key, "ubus_timeout") == 0) 
                {
                    conf->ubus_timeout = parseint(value, true, UBUS_TIMEOUT);
                }
                else 
                {
                    log_message(LOG_WARNING, "Unknown configuration option: '%s'\r\n", key);
                }
            }
        }

        if (errno != 0) {
            free(conf);
            fclose(fd);
            return false;
        }
        
        fclose(fd);
        return true;
    }
    return false;
}


/**
 * Free the parsed configuration data
 */
void config_free() {
    // TODO free the used strings
    
    free(conf);
}

/**
 * Pretty print server configuration to stdout
 */
void config_print() {
    printf("Breakout server configuration\r\n");
    printf("-----------------------------\r\n\r\n");
    printf("Run as daemon: %s\r\n\r\n", conf->daemon ? "yes" : "no");
    
    printf("Listen port: %s\r\n", conf->listen_port);
    printf("Database location: %s\r\n", conf->database);
    printf("Keep alive time: %d\r\n", conf->keep_alive_time);
    printf("Network timeout: %d\r\n\r\n", conf->network_timeout);
    
    printf("Index file: %s\r\n", conf->index_file);
    printf("Document root: %s\r\n", conf->document_root);
    printf("API prefix: %s\r\n", conf->api_prefix);
    printf("API prefix length: %d\r\n\r\n", conf->api_str_len);

    printf("Public firmware uri: %s\r\n", conf->public_firmware_uri);
    printf("Firmware download path: %s\r\n", conf->firmware_download_path);
    printf("Firmware file name: %s\r\n\r\n", conf->firmware_file_name);
    
    printf("µBus timeout: %d\r\n", conf->ubus_timeout);
}
