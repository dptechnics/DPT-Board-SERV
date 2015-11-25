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
 * File:   wifi.h
 * Created on November 12, 2014, 6:29 PM
 */

#ifndef WIFI_H
#define	WIFI_H

#include <stdbool.h>
#include "iwinfo.h"

#define UCI_PTR_STR_BUFF_SIZE               512

#define UCI_CLIENT_WIFI                     "client"
#define UCI_CFG_WIFI                        "cfg"

/*
 * UCI configuration strings
 */
#define UCI_WIFI_SET_SSID                   "wireless.%s.ssid=%s"
#define UCI_WIFI_GET_SSID                   "wireless.%s.ssid"
#define UCI_WIFI_SET_KEY                    "wireless.%s.key=%s"
#define UCI_WIFI_SET_ENC                    "wireless.%s.encryption=%s"
#define UCI_WIFI_SET_DIS                    "wireless.%s.disabled=%s"
#define UCI_WIFI_GET_DIS                    "wireless.%s.disabled"

/*
 * Possible DPT-Board WiFi adapters, declared in wifi.c 
 */
#define WIFI_ADPATER_ARRAY_LEN              2
extern const char* wifi_adapters[WIFI_ADPATER_ARRAY_LEN];

/* 
 * This variable is true when the user attempted to connect to a 
 * new client network. When this connection fails the board
 * will not try to reconnect. 
 */
extern bool new_wifi_attempt;

/*
 * This variable is true when the 'new_wifi_attempt' variable
 * may be cleared. 
 */
extern bool clear_new_wifi_command;

/*
 * WiFi adapter information structure 
 */
struct wifi_adapter {
    char name[10];                          /* The adapter name (wlan0 or wlan0-1) */
    char ssid[IWINFO_ESSID_MAX_SIZE + 1];   /* The associated network SSID */
    bool enabled;                           /* If this network is enabled or disabled */
    bool connected;                         /* If this network is connected or not */
    int channel;                            /* The current network channel */
    int signal;                             /* The current network signal in dBm */
    int quality;                            /* The quality of the network in % */
    int mode;                               /* The mode this adapter works in STA/AP */
};

/*
 * WiFi adapter information summary struct
 */
struct wifi_adapter_info {
    pthread_mutex_t lock;
    struct wifi_adapter cfg;
    struct wifi_adapter client;
};

/*
 * Adapter information
 */
extern struct wifi_adapter_info wifi_adapt_info;

/**
 * Initialize the WiFi module
 */
void wifi_init();

/**
 * Scan for available WiFi networks and store them
 * in memory.
 */
void wifi_scan_networks();

/**
 * Get summary about client WiFi and config WiFi and
 * store them in memory. 
 */
void wifi_make_summary();

/**
 * Change the SSID of a wireless network.
 * @param network the wireless network SSID to change.
 * @param ssid the new SSID.
 * @return true on success, false on failure.
 */
bool wifi_set_ssid(const char* network, const char* ssid);

/**
 * Set-up encryption of a wireless network.
 * @param network the wireless network SSID to change.
 * @param type the encryption type to set up.
 * @param key the network key to use.
 * @return true on success false on error. 
 */
bool wifi_set_encryption(const char* network, int type, const char* key);

/**
 * Set the state of a network to enabled or disabled.
 * @param network the wireless network SSID to change. 
 * @param enabled true when the network should be enabled, else false.
 * @return true on success false on error.
 */
bool wifi_set_state(const char* network, bool enabled);

/**
 * Connect the board to a WiFi network as a client.
 * @param ssid the SSID to connect to.
 * @param key the type of security to use.
 * @param password the password from the WiFi network.
 * @param username the username, this is necessary for radius.
 * @return true on success.
 */
bool wifi_client_connect(const char* ssid, const int security, const char* key, const char* username);

/**
 * Reload networking and WiFi configuration by restarting WiFi hardware
 * @return true on success, false on failure
 */
bool wifi_restart();

/**
 * Returns true when the client network is enabled.
 * @return true when the network section is enabled.
 */
bool wifi_client_iface_enabled();

/**
 * Returns true when the configuration network is enabled.
 * @return true when the network section is enabled.
 */
bool wifi_cfg_iface_enabled();

/**
 * Returns true when the client network is enabled.
 * @return true when the client network is enabled.
 */
bool wifi_client_network_connected();

#endif

