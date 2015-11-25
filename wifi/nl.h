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
 * File:   nl.h
 * Created on May 18, 2015, 3:13 PM
 */

#ifndef NL_H
#define	NL_H

#include <stdbool.h>
#include <pthread.h>

/* User security */
#define NL_WIFI_SECURITY_NONE       0
#define NL_WIFI_SECURITY_WEP        1
#define NL_WIFI_SECURITY_WPA_P      2
#define NL_WIFI_SECURITY_WPA2_P     3
#define NL_WIFI_SECURITY_WPA_E      4
#define NL_WIFI_SECURITY_WPA2_E     5

/* WiFi beacon or probe packet information element numbers */
#define IE_RSN                  48
#define IE_WPA                  221
#define IE_VENDOR_WPA           1

/* WiFi capabilities */
#define WLAN_CAPABILITY_PRIVACY		(1<<4)

extern char* nl_sec_map[6];
extern struct nl_wifi_network_list wifi_list;

/**
 * State of the Netlink kernel socket connection
 */
struct nl_state {
    struct nl_sock *nl_sock;
    int nl80211_id;
    signed long long wlan_devid;
};

/**
 * WiFi network structure 
 */
struct nl_wifi_network {
    char ssid[33];
    int signal;
    int quality;
    int security;
    struct nl_wifi_network *next;
};

/**
 * WiFi network list 
 */
struct nl_wifi_network_list {
    pthread_mutex_t lock;
    bool editing;
    struct nl_wifi_network *list;
};

/**
 * Family handler arguments
 */
struct fmly_handler_args {
    const char *group;
    int id;
};

/**
 * Wait event data
 */
struct wait_event {
    int n_cmds;
    const unsigned int *cmds;
    unsigned int cmd;
};

/**
 * Clear all entries in the WiFi network list and start editing.
 * @param w_list the list to clear.
 */
void nl_wifi_start_edit(struct nl_wifi_network_list *w_list);

/**
 * Add a a WiFi network to the list.
 * @param w_list the list to add the WiFi network to.
 * @param w_net the network to add. 
 */
void nl_wifi_list_add(struct nl_wifi_network_list *w_list, struct nl_wifi_network *w_net);

/**
 * Turn off the editing flag.
 * @param w_list the list to stop editing.
 */
void nl_wifi_list_stop_edit(struct nl_wifi_network_list *w_list);

/**
 * Initializer for the the Netlink connection.
 * @param state the Netlink connection state struct.
 * @return true on success, false on error.
 */
bool nl_init(struct nl_state *state);

/**
 * Clean the Netlink connection state.
 * @param state the Netlink connection state struct.
 */
void nl_clean(struct nl_state *state);

/**
 * Trigger a WLAN scan on the default WLAN adapter
 * @param nlstate the Netlink connection state.
 * @return true on success, false on error. 
 */
bool nl_trigger_scan(struct nl_state *nlstate);

/**
 * Dump the WiFi scan results into memory.
 * @param nlstate the Netlink connection state.
 * @return true on success, false on error.
 */
bool nl_dump_scan(struct nl_state *nlstate);

/**
 * Scan for WiFi networks in reach without shutting down WiFi connection
 * @return true on success, false on error. 
 */
bool nl_scan_networks();

#endif

