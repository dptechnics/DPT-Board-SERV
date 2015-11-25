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
 * File:   bluecherry.c
 * Created on April 9, 2015, 4:33 AM
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <json-c/json.h>
#include <curl/curl.h>

#include "../logger.h"
#include "../config.h"
#include "bluecherry.h"

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
 * Create public and private key for SSH connection.
 * @return true on success. 
 */
bool bluecherry_create_sshkey() {
    /* Generate public key */
    system("rm /root/.ssh/id_rsa /root/.ssh/id_rsa.pub");
    if (system("ssh-keygen -t rsa -q -f /root/.ssh/id_rsa -N '' -C ''") != 0) {
        return false;
    }

    return true;
}

/**
 * Get the board public SSH key
 * @param key the key string to fill up.
 * @return true on success, false otherways. 
 */
bool bluecherry_get_pubkey(char* key) {
    FILE *fp = fopen("/root/.ssh/id_rsa.pub", "r");

    if (fp != NULL) {
        size_t read = fread(key, sizeof (char), 399, fp);

        if (read == 0) {
            fclose(fp);
            return false;
        } else {
            fclose(fp);
            
            log_message(LOG_DEBUG, "Read SSH key: '%s'\r\n", key);
            return true;
        }
    }

    return false;
}

/**
 * Write a device id and device key to the dpt-connector configuration.
 * @param devid the device ID to write.
 * @param devkey the device key to write.
 * @return  true on success.
 */
bool bluecherry_write_con_config(const char* devid, const char* devkey) {
    char com_buff[512];
    
    // Check for bad input 
    //if(!isalnum(devid) || !isalnum(devkey))
    //{
    //    log_message(LOG_WARNING, "Detected bad input for system command bluecherry.c:101\r\n");
    //    return false;
    //}
    

    /* Prepare command */
    sprintf(com_buff, "grep -q '^devid' /etc/config/dpt-connector || echo 'devid %s' >> /etc/config/dpt-connector && sed -i 's/^devid.*/devid %s/' /etc/config/dpt-connector", devid, devid);
    if (system(com_buff) != 0) {
        return false;
    }

    sprintf(com_buff, "grep -q '^devkey' /etc/config/dpt-connector || echo 'devkey %s' >> /etc/config/dpt-connector && sed -i 's/^devkey.*/devkey %s/' /etc/config/dpt-connector", devkey, devkey);
    if (system(com_buff) != 0) {
        return false;
    }

    return true;
}

/**
 * Get the device type from the dpt-connector configuration. 
 * @return the device key, freed by the user, or NULL on error.
 */
char* bluecherry_get_dev_type() {
    /* Read buffer */
    char buffer[512];
    FILE * fd;
    char* key;
    char* value;
    char* retval;
    errno = 0;
    
    /* Parse configuration file if any */
    if ((fd = fopen("/etc/config/dpt-connector", "r")) != NULL) {

        while (fgets(buffer, 512, fd) != NULL) {
            /* Ignore lines starting with '#', ';' or whitespace  */
            if (buffer[0] != '#' && buffer[0] != ';' && buffer[0] != ' ' && buffer[0] != '\t' && buffer[0] != '\r' && buffer[0] != '\n') {
                char* trimmed = trimwhitespace(buffer);
                key = strtok(trimmed, " \t");
                value = strtok(NULL, " ,.-");
                
                log_message(LOG_DEBUG, "Reading configuration: (%s = %s)\r\n", key, value);
                
                if (strcmp(key, "typeid") == 0) {
                    retval = (char*) malloc(strlen(value) * sizeof (char));
                    strcpy(retval, value);
                    fclose(fd);
                    return retval;
                }
            }

            if (errno != 0) {
                fclose(fd);
                return NULL;
            }
        }
        fclose(fd);
    }

    return NULL;
}

/**
 * Append a BlueCherry edge server key to the known hosts file.
 * @param host the host key to add.
 * @return true if success.
 */
bool bluecherry_add_known_host(char* host) {
    /* Read buffer */
    FILE * fd;

    /* Parse configuration file if any */
    if ((fd = fopen("cat /root/.ssh/known_hosts", "a")) != NULL) {
        fprintf(fd, "\n%s", host);
        fclose(fd);
        return true;
    }

    /* Could not open file */
    return false;
}

/**
 * Login to the BlueCherry platform site
 * @param username the username to login with
 * @param password the password to login with. 
 * @return true when login is successful, false otherways 
 */
bool bluecherry_login(const char* username, const char* password) {
    CURL *curl; /* cURL handle */
    CURLcode res; /* POST result code */
    char urlbuff[512] = {0}; /* The URL buffer */
    char buff[512] = {0}; /* POST data buffer */
    long http_resp; /* HTTP response code */
    bool retvalue = false; /* Function return value */
    char errbuff[CURL_ERROR_SIZE] = {0}; /* Buffer for cURL error messages */

    /* Initialize cURL */
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        /* Prepare url and post data buffer */
        snprintf(urlbuff, 512, BLUECHERRY_URL_LOGIN "/%s", username);
        snprintf(buff, 512, "email=%s&password=%s", username, password);
        log_message(LOG_DEBUG, "BlueCherry login for: %s, pass: ***\r\n", username);

        /* Set correct post options */
        curl_easy_setopt(curl, CURLOPT_URL, urlbuff);
        curl_easy_setopt(curl, CURLOPT_CAINFO, "/etc/curlssl/cacert.pem");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buff);
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, BLUECHERRY_COOKIE_FILE);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuff);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, CURL_USER_AGENT);

        /* Perform login */
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            log_message(LOG_ERROR, "Could not POST login data to bluecherry platform: (err: #%d) %s\r\n", res, errbuff);
            goto end;
        }

        curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &http_resp);
        retvalue = http_resp == 200;

end:
        /* Cleanup cURL */
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return retvalue;
}

/**
 * Helper function for libcURL. 
 * @param contents the contents to write to the memory.
 * @param size the size of a memory member. 
 * @param nmemb the number of members.
 * @param userp the memory struct to write to.
 * @return 
 */
static size_t _write_mem_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct curl_mem_struct *mem = (struct curl_mem_struct *) userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        /* out of memory! */
        log_message(LOG_ERROR, "Not enough memory to save HTTP response\r\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

/**
 * Start the BlueCherry device initialisation
 * @param username the username to login with
 * @param password the password to login with. 
 * @return bluecherry init status
 */
bluecherry_init_state bluecherry_device_init(const char* username, const char* password) {
    CURL *curl;                                                 /* cURL handle */
    CURLcode res;                                               /* POST result code */
    char urlbuff[512] = {0};                                    /* The URL buffer */
    char* buff;                                                 /* The cURL data buffer */
    long http_resp;                                             /* HTTP response code */
    char errbuff[CURL_ERROR_SIZE] = {0};                        /* Buffer for cURL error messages */
    struct curl_mem_struct chunk;                               /* Structure that contains the HTTP response */
    char* typeid;                                               /* This device type id */
    const char* devid;                                          /* This device device id */
    const char* devkey;                                         /* This device device key */
    char* sshkey = NULL;                                        /* This device public key */
    bluecherry_init_state result = BLUECHERRY_INIT_ERR_UNKNOWN; /* The BlueCherry result */

    /* Initialize memory structure */
    chunk.memory = malloc(1);
    chunk.size = 0;

    /* Get the BlueCherry device type */
    typeid = bluecherry_get_dev_type();
    if (typeid == NULL) {
        log_message(LOG_ERROR, "Cannot read device type id\r\n");
        return result;
    }

    /* First login to the BlueCherry system*/
    if (!bluecherry_login(username, password)) {
        log_message(LOG_ERROR, "Cannot login into the BlueCherry platform\r\n");
        free(typeid);
        return BLUECHERRY_INIT_WRONG_CREDS;
    }

    /* Initialize cURL */
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        /* Prepare url buffer */
        snprintf(urlbuff, 512, BLUECHERRY_URL_INIT, typeid);
        log_message(LOG_DEBUG, "BlueCherry device init, type ID: %s\r\n", typeid);

        /* Set correct GET options */
        curl_easy_setopt(curl, CURLOPT_URL, urlbuff);
        curl_easy_setopt(curl, CURLOPT_CAINFO, "/etc/curlssl/cacert.pem");
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, BLUECHERRY_COOKIE_FILE);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuff);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, CURL_USER_AGENT);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _write_mem_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);

        /* Perform device initialization */
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            log_message(LOG_ERROR, "Could not start device initialization: (err: #%d) %s\r\n", res, errbuff);
            curl_easy_cleanup(curl);
            goto end;
        }

        /* Check if we got 200 Ok response */
        curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &http_resp);
        if (http_resp != 200) {
            log_message(LOG_ERROR, "Could not start device initialization\r\n");
            curl_easy_cleanup(curl);
            goto end;
        }

        /* Now parse device id and device key */
        json_object *in_obj = json_tokener_parse(chunk.memory);
        
        json_object *j_result = NULL;
        if(!json_object_object_get_ex(in_obj, "result", &j_result)) {
            if (json_object_put(in_obj) != 1) {
                log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
            }
            curl_easy_cleanup(curl);
            goto end;
        }
        
        /* Now check result */
        if(!json_object_get_boolean(j_result)) {
            json_object *j_reason = NULL;
            if(json_object_object_get_ex(in_obj, "reason", &j_reason)){
                const char *reason = json_object_get_string(j_reason);
                log_message(LOG_ERROR, "Could not register device: %s\r\n", reason);
                if(strncmp("maxreached", reason, 10) == 0) {
                    result = BLUECHERRY_INIT_MAX_REACHED;
                }
            }
            if (json_object_put(in_obj) != 1) {
                log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
            }
            curl_easy_cleanup(curl);
            goto end;
        }
        
        json_object *j_devid = NULL;
        json_object *j_devkey = NULL;
        if(!json_object_object_get_ex(in_obj, "devid", &j_devid) ||
           !json_object_object_get_ex(in_obj, "devkey", &j_devkey))
        {
            result = BLUECHERRY_INIT_ERR_UNKNOWN;
            if (json_object_put(in_obj) != 1) {
                log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
            }
            curl_easy_cleanup(curl);
            goto end;
        }
        
        /* We have got our device id and device key at this point */
        devid = json_object_get_string(j_devid);
        devkey = json_object_get_string(j_devkey);

        /* Cleanup previous cURL */
        curl_easy_cleanup(curl);

        /* Now generate SSH key and send it to the platform */
        curl = curl_easy_init();
        if (curl) {
            
            /* Key is 380 characters long, reserve memory */
            sshkey = (char*) calloc(400, sizeof (char));
            if (!bluecherry_create_sshkey() || !bluecherry_get_pubkey(sshkey)) {
                log_message(LOG_ERROR, "Could not generate SSH keypair\r\n");
                goto end;
            }

            buff = (char*) malloc(4096 * sizeof (char));
            char* trimkey = trimwhitespace(sshkey);
            char* enckey = curl_easy_escape(curl, trimkey, strlen(trimkey));
            if(enckey == NULL) {
                goto keyend;
            }
            
            log_message(LOG_DEBUG, "URL encoded SSH key: '%s'\r\n", enckey);
            snprintf(buff, 512, "typeid=%s&devid=%s&sshkey=%s", typeid, devid, enckey);
            
            /* Set correct PUT options */
            curl_easy_setopt(curl, CURLOPT_URL, BLUECHERRY_URL_SSH);
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_CAINFO, "/etc/curlssl/cacert.pem");
            curl_easy_setopt(curl, CURLOPT_COOKIEFILE, BLUECHERRY_COOKIE_FILE);
            curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuff);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, CURL_USER_AGENT);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buff);

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                log_message(LOG_ERROR, "Could not start device initialization: (err: #%d) %s\r\n", res, errbuff);
                goto keyend;
            }

            /* Check if we got 200 Ok response */
            curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &http_resp);
            if (http_resp != 200) {
                log_message(LOG_ERROR, "Could not send this device public SSH key\r\n");
                goto keyend;
            }
            
            /* Now save the device id and device key in the connector configuration */
            bluecherry_write_con_config(devid, devkey);
            result = BLUECHERRY_INIT_SUCCESS;
            
            curl_free(enckey);
keyend:
            /* Fee some memory */
            free(buff);
            free(sshkey);
            if (json_object_put(in_obj) != 1) {
                log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
            }

        }
        /* Cleanup cURL */
        curl_easy_cleanup(curl);
    }
    
end:
    free(typeid);
    curl_global_cleanup();
    return result;
}

/**
 * Get the status of the bluecherry connection.
 * @return the current BlueCherry connection status.
 */
bluecherry_state bluecherry_status()
{
    /* Check if the configuration is complete */
    if (system("cat " BLUECHERRY_CONFIG_FILE " | grep 'devkey'") != 0) {
        return BLUECHERRY_NOT_INITIALIZED;
    } else {
        /* Check if the connection is present */
        if (system("ps -w | grep ':localhos[t]:80'") == 0) {
            /* The connection is present */
            return BLUECHERRY_CONNECTED;
        } else {
            /* The connection is not available */
            return BLUECHERRY_NO_INET;
        }
    }
}
        
