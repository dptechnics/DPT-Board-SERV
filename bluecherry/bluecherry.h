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
 * File:   bluecherry.h
 * Created on April 9, 2015, 3:23 AM
 */

#ifndef BLUECHERRY_H
#define	BLUECHERRY_H

/* Bluecherry data */
#define BLUECHERRY_COOKIE_FILE      "/tmp/curlcookies"

#define BLUECHERRY_URL_LOGIN        "https://platform.bluecherry.io/login"
#define BLUECHERRY_URL_INIT         "https://platform.bluecherry.io/deviface/%s/init"
#define BLUECHERRY_URL_SSH          "https://platform.bluecherry.io/deviface/sshkey"

/*
 * Memory structure to save HTTP responses in. 
 */
struct curl_mem_struct {
  char *memory;
  size_t size;
};

/*
 * BlueCherry connection state
 */
typedef enum {
    BLUECHERRY_NOT_INITIALIZED = 1,
    BLUECHERRY_NO_INET = 2,
    BLUECHERRY_CONNECTED = 3
} bluecherry_state;

/*
 * BlueCherry device connect state
 */
typedef enum {
    BLUECHERRY_INIT_SUCCESS = 0,
    BLUECHERRY_INIT_MAX_REACHED = 1,
    BLUECHERRY_INIT_WRONG_CREDS = 2,
    BLUECHERRY_INIT_ERR_UNKNOWN = 3
} bluecherry_init_state;

/**
 * Create public and private key for SSH connection.
 * @return true on success. 
 */
bool bluecherry_create_sshkey();

/**
 * Get the board public SSH key.
 * @param key the key string to fill up.
 * @return true on success, false otherways. 
 */
bool bluecherry_get_pubkey(char* key);

/**
 * Write a device id and device key to the dpt-connector configuration.
 * @param devid the device ID to write.
 * @param devkey the device key to write.
 * @return  true on success.
 */
bool bluecherry_write_con_config(const char* devid, const char* devkey);

/**
 * Get the device type from the dpt-connector configuration. 
 * @return the device key, freed by the user, or NULL on error.
 */
char* bluecherry_get_dev_type();

/**
 * Append a BlueCherry edge server key to the known hosts file.
 * @param host the host key to add.
 * @return true if success.
 */
bool bluecherry_add_known_host(char* host);

/**
 * Login to the BlueCherry platform site
 * @param username the username to login with
 * @param password the password to login with. 
 * @return true when login is successful, false otherways 
 */
bool bluecherry_login(const char* username, const char* password);

/**
 * Start the BlueCherry device initialisation
 * @param username the username to login with
 * @param password the password to login with. 
 * @return bluecherry init status.
 */
bluecherry_init_state bluecherry_device_init(const char* username, const char* password);

/**
 * Get the status of the bluecherry connection.
 * @return the current BlueCherry connection status.
 */
bluecherry_state bluecherry_status();
#endif

