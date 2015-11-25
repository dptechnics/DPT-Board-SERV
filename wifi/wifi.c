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
 * 
 * File:   wifi.c
 * Created on November 12, 2014, 6:31 PM
 */

#include <stdint.h>
#include <stdbool.h>
#include <uci.h>
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include <stdlib.h>

#include "nl.h"
#include "wifi.h"
#include "iwinfo.h"
#include "../config.h"
#include "../logger.h"
#include "../network/network.h"

/*
 * Possible DPT-Board WiFi adapters, declared in wifi.c 
 */
const char* wifi_adapters[WIFI_ADPATER_ARRAY_LEN] = {
    "wlan0",
    "wlan0-1"
};

/*
 * WiFi adapter information
 */
struct wifi_adapter_info wifi_adapt_info;

/**
 * Scan for available wifi networks and store them
 * in memory.
 */
void wifi_scan_networks()
{
    nl_scan_networks();
}

/**
 * Helper that reads the SSID and enabled state for a network. 
 * @param network the network configuration section name. 
 * @param adaptinfo the adapter information structure 
 */
static void _wifi_read_ssid_enstate(const char* network, struct wifi_adapter *adaptinfo)
{
    struct uci_context *u_ctx = NULL;
    struct uci_ptr u_ptr;
    char* err_str;
    
    /* Create space for pointer buffer */
    char ptr_buff[UCI_PTR_STR_BUFF_SIZE];
    
    if(!(u_ctx = uci_alloc_context())) {
        log_message(LOG_ERROR, "Could not allocate UCI buffer for reading WiFi adapter SSID and enabled state\r\n");
        return;
    }

    /* Prepare pointer to get network disabled state */
    int result = snprintf(ptr_buff, UCI_PTR_STR_BUFF_SIZE, UCI_WIFI_GET_DIS, network);
    if(result < 0 || result >= UCI_PTR_STR_BUFF_SIZE) {
        log_message(LOG_ERROR, "Error while preparing UCI command buffer\r\n");
        goto error;
    }
    
    /* Lookup the pointer */
    if(uci_lookup_ptr(u_ctx, &u_ptr, ptr_buff, false) != UCI_OK) {
        log_message(LOG_ERROR, "Could not lookup UCI pointer '%s' while getting WiFi disabled state\r\n", ptr_buff);   
        goto error;
    }
    
    if(u_ptr.value) {
        log_message(LOG_ERROR, "The pointer value is not null on UCI lookup\r\n");
        goto error;
    }   
    if(!(u_ptr.flags && UCI_LOOKUP_COMPLETE)) {
        uci_unload(u_ctx, u_ptr.p);
        goto error;
    }
    
    if(u_ptr.last->type == UCI_TYPE_OPTION && u_ptr.o->type == UCI_TYPE_STRING) {
        /* True when disabled is 0 */
        adaptinfo->enabled = u_ptr.o->v.string[0] == '0';
    } else {
        adaptinfo->enabled = false;
    }
    
    /* Unload the pointer */
    uci_unload(u_ctx, u_ptr.p);
    
    /* Prepare pointer to get network SSID */
    result = snprintf(ptr_buff, UCI_PTR_STR_BUFF_SIZE, UCI_WIFI_GET_SSID, network);
    if(result < 0 ) {
        log_message(LOG_ERROR, "Encoding error in snprintf while getting WiFi SSID \r\n");
        goto error;
    }
    if (result >= UCI_PTR_STR_BUFF_SIZE) {
        log_message(LOG_ERROR, "'network' to long for buffer while getting WiFi SSID \r\n");
        goto error;
    }
    
    /* Lookup the pointer */
    if(uci_lookup_ptr(u_ctx, &u_ptr, ptr_buff, false) != UCI_OK) {
        log_message(LOG_ERROR, "Could not lookup UCI pointer while getting WiFi SSID\r\n");   
        log_message(LOG_DEBUG, "UCI pointer string: '%s'\r\n", ptr_buff);
        goto error;
    }
    
    if(u_ptr.value) {
        log_message(LOG_ERROR, "The pointer value is not null on UCI lookup\r\n");
        goto error;
    }
    
    if(!(u_ptr.flags && UCI_LOOKUP_COMPLETE)) {
        log_message(LOG_DEBUG, "UCI lookup not complete\r\n");
        uci_unload(u_ctx, u_ptr.p);
        goto error;
    }
    
    if(u_ptr.last->type == UCI_TYPE_OPTION && u_ptr.o->type == UCI_TYPE_STRING) {
        strncpy(adaptinfo->ssid, u_ptr.o->v.string, 32);
    } else {
        adaptinfo->ssid[0] = '\0';
    }
    
    /* Unload the pointer */
    uci_unload(u_ctx, u_ptr.p);
    
    /* Free UCI context */
    uci_free_context(u_ctx);
    
    return;
error:
    uci_get_errorstr(u_ctx, &err_str, "UCI error");
    log_message(LOG_ERROR, "%s\r\n", err_str);
    free(err_str);
    uci_free_context(u_ctx);
}

/**
 * Get summary about client WiFi and config WiFi and
 * store them in memory. 
 */
void wifi_make_summary()
{   
    const struct iwinfo_ops *iw;
    int i;
    
    bool got_cfg_adapter = false;
    bool got_client_adapter = false;
    
    /* Get the WiFi adapter summary lock */
    pthread_mutex_lock(&(wifi_adapt_info.lock));
    
    /* Get SSID and enabled info about the adapters */
    _wifi_read_ssid_enstate(UCI_CFG_WIFI, &(wifi_adapt_info.cfg));
    _wifi_read_ssid_enstate(UCI_CLIENT_WIFI, &(wifi_adapt_info.client));
    
    /* Get WiFi data if enabled */
    for(i = 0; i < WIFI_ADPATER_ARRAY_LEN; ++i) 
    {
        /* Try to open iwinfo backend for adapter */
        iw = iwinfo_backend(wifi_adapters[i]);
        if(!iw) {
            continue;
        }
        
        char buf[IWINFO_ESSID_MAX_SIZE + 1] = {0};
        if(iw->ssid(wifi_adapters[i], buf)) {
            memset(buf, 0, sizeof(buf));
        }
        
        /* Now select the correct adapter based on it's SSID */
        struct wifi_adapter *adaptinfo = NULL;
        if(strcmp(buf, wifi_adapt_info.cfg.ssid) == 0) {
            adaptinfo = &(wifi_adapt_info.cfg);
            got_cfg_adapter = true;
        } else if(strcmp(buf, wifi_adapt_info.client.ssid) == 0) {
            adaptinfo = &(wifi_adapt_info.client);
            got_client_adapter = true;
        }
        
        if(adaptinfo == NULL) {
            continue;
        }
        
        /* If we reach here the adapter is connected */
        adaptinfo->connected = true;
        
        /* Add adapter name */
        strcpy(adaptinfo->name, wifi_adapters[i]);
        
        /* Add channel */
        int ch;
        if(iw->channel(wifi_adapters[i], &ch)) {
            ch = -1;
        }
        adaptinfo->channel = ch;
        
        /* Add signal */
        int sig;
        if(iw->signal(wifi_adapters[i], &sig)) {
            sig = 0;
        }
        adaptinfo->signal = sig;
        
        /* Add quality */
        int quality;
        int quality_max;
        if(iw->quality(wifi_adapters[i], &quality) || iw->quality_max(wifi_adapters[i], &quality_max)) {
            quality = -1;
        } else {
            quality = (int) (((double) quality/ (double) quality_max) * 100);
        }
        adaptinfo->quality = quality;
        
        /* Add mode */
        int mode;
        if(iw->mode(wifi_adapters[i], &mode)) {
            mode = IWINFO_OPMODE_UNKNOWN;
        }
        adaptinfo->mode = mode;
        
        /* Clear data */
        iwinfo_finish();
    }
    
    /* Reset data on adapter which are not connected */
    struct wifi_adapter *adaptinfo = NULL;
    if(!got_cfg_adapter) {
        adaptinfo = &(wifi_adapt_info.cfg);
    } else if(!got_client_adapter) {
        adaptinfo = &(wifi_adapt_info.client);
    }
    
    if(adaptinfo != NULL) {
        adaptinfo->connected = false;
        adaptinfo->channel = -1;
        adaptinfo->quality = -1;
        adaptinfo->mode = IWINFO_OPMODE_UNKNOWN;
    }
    
    /* Release the lock */
    pthread_mutex_unlock(&(wifi_adapt_info.lock));
}

/**
 * Change the SSID of a wireless network
 * @param network the wireless network SSID to change
 * @param ssid the new SSID
 * @return true on success, false on failure
 */
bool wifi_set_ssid(const char* network, const char* ssid)
{
    struct uci_context *u_ctx = NULL;
    struct uci_ptr u_ptr;
    char* err_str;
    
    char ptr_buff[UCI_PTR_STR_BUFF_SIZE];
    
    u_ctx = uci_alloc_context();
    if(!u_ctx) {
        log_message(LOG_ERROR, "Could not allocate UCI context for changing WiFi SSID\r\n");
        return false;
    }
    
    int result = snprintf(ptr_buff, UCI_PTR_STR_BUFF_SIZE, UCI_WIFI_SET_SSID, network, ssid);
    if(result < 0 ) {
        log_message(LOG_ERROR, "Encoding error in snprintf while setting WiFi SSID");
        goto error;
    }
    if (result >= UCI_PTR_STR_BUFF_SIZE) {
        log_message(LOG_ERROR, "'network' and 'ssid' to long for buffer while setting WiFi SSID\r\n");
        goto error;
    }
    
    /* Set and commit the new SSID */
    if(uci_parse_ptr(u_ctx, &u_ptr, ptr_buff) != UCI_OK){
        log_message(LOG_ERROR, "Could not parse UCI pointer while setting WiFi SSID\r\n");   
        log_message(LOG_DEBUG, "UCI pointer string: '%s'\r\n", ptr_buff);
        goto error;
    }
    
    if(uci_set(u_ctx, &u_ptr) != UCI_OK) {
        log_message(LOG_ERROR, "Could not UCI set new SSID for WiFi network:\r\n");
        goto error;    
    }
    
    if(uci_commit(u_ctx, &u_ptr.p, false) != UCI_OK) {
        log_message(LOG_ERROR, "Could not commit UCI changes while setting WiFi SSID:\r\n");
        goto error; 
    }
    
    /* Free UCI context */
    uci_free_context(u_ctx);
    return true;
    
error:
    uci_get_errorstr(u_ctx, &err_str, "UCI error");
    log_message(LOG_ERROR, "%s\r\n", err_str);
    free(err_str);
    uci_free_context(u_ctx);
    return false;             
}

/**
 * Set-up encryption of a wireless network.
 * @param network the wireless network SSID to change.
 * @param type the encryption type to set up.
 * @param key the network key to use.
 * @return true on success false on error. 
 */
bool wifi_set_encryption(const char* network, int type, const char* key)
{
    struct uci_context *u_ctx = NULL;
    struct uci_ptr u_ptr;
    char* err_str;
    int result = 0;
    
    char ptr_buff[UCI_PTR_STR_BUFF_SIZE];
    
    u_ctx = uci_alloc_context();
    if(!u_ctx) {
        log_message(LOG_ERROR, "Could not allocate UCI context for changing WiFi encryption\r\n");
        return false;
    }
    
    /* Set up client network encryption */
    switch(type) {
        case NL_WIFI_SECURITY_NONE:
            result = snprintf(ptr_buff, UCI_PTR_STR_BUFF_SIZE, UCI_WIFI_SET_ENC, network, "none");
            break;
        case NL_WIFI_SECURITY_WEP:
            result = snprintf(ptr_buff, UCI_PTR_STR_BUFF_SIZE, UCI_WIFI_SET_ENC, network, "wep");
            break;
        case NL_WIFI_SECURITY_WPA_P:
        case NL_WIFI_SECURITY_WPA_E:
            result = snprintf(ptr_buff, UCI_PTR_STR_BUFF_SIZE, UCI_WIFI_SET_ENC, network, "psk");
            break;
        case NL_WIFI_SECURITY_WPA2_P:
        case NL_WIFI_SECURITY_WPA2_E:
            result = snprintf(ptr_buff, UCI_PTR_STR_BUFF_SIZE, UCI_WIFI_SET_ENC, network, "psk2");
            break;
    }
    if(result < 0 || result >= UCI_PTR_STR_BUFF_SIZE) {
        log_message(LOG_ERROR, "Encoding error in snprintf while setting encryption\r\n");
        goto error;
    }
    if(uci_parse_ptr(u_ctx, &u_ptr, ptr_buff) != UCI_OK){
        log_message(LOG_ERROR, "Could not parse UCI pointer while setting encryption\r\n");   
        log_message(LOG_DEBUG, "UCI pointer string: '%s'\r\n", ptr_buff);
        goto error;
    }
    if(uci_set(u_ctx, &u_ptr) != UCI_OK) {
        log_message(LOG_ERROR, "Could not UCI set WiFi encryption\r\n");
        goto error;    
    }
    
    /* Set up client network key */
    switch(type) {
        case NL_WIFI_SECURITY_NONE:
            result = snprintf(ptr_buff, UCI_PTR_STR_BUFF_SIZE, UCI_WIFI_SET_KEY, network, "");
            break;
        case NL_WIFI_SECURITY_WEP:
            log_message(LOG_ERROR, "WEP is not yet implemented\n\r");
            break;
        case NL_WIFI_SECURITY_WPA_P:
        case NL_WIFI_SECURITY_WPA2_P:
            result = snprintf(ptr_buff, UCI_PTR_STR_BUFF_SIZE, UCI_WIFI_SET_KEY, network, key);
            break;
        case NL_WIFI_SECURITY_WPA_E:
        case NL_WIFI_SECURITY_WPA2_E:
            log_message(LOG_ERROR, "WPA-Enterprise is not yet implemented\n\r");
            goto error;
            break;
    }
    if(result < 0 || result >= UCI_PTR_STR_BUFF_SIZE) {
        log_message(LOG_ERROR, "Encoding error in snprintf while setting key\r\n");
        goto error;
    }
    if(uci_parse_ptr(u_ctx, &u_ptr, ptr_buff) != UCI_OK){
        log_message(LOG_ERROR, "Could not parse UCI pointer while setting key\r\n");   
        log_message(LOG_DEBUG, "UCI pointer string: '%s'\r\n", ptr_buff);
        goto error;
    }
    if(uci_set(u_ctx, &u_ptr) != UCI_OK) {
        log_message(LOG_ERROR, "Could not UCI set client WiFi key\r\n");
        goto error;    
    }
    
    if(uci_commit(u_ctx, &u_ptr.p, false) != UCI_OK) {
        log_message(LOG_ERROR, "Could not commit UCI changes while setting WiFi encryption:\r\n");
        goto error; 
    }
    
    /* Free UCI context */
    uci_free_context(u_ctx);
    return true;
    
error:
    uci_get_errorstr(u_ctx, &err_str, "UCI error");
    log_message(LOG_ERROR, "%s\r\n", err_str);
    free(err_str);
    uci_free_context(u_ctx);
    return false;      
}

/**
 * Set the state of a network to enabled or disabled.
 * @param network the wireless network SSID to change. 
 * @param enabled true when the network should be enabled, else false.
 * @return true on success false on error.
 */
bool wifi_set_state(const char* network, bool enabled)
{
    struct uci_context *u_ctx = NULL;
    struct uci_ptr u_ptr;
    char* err_str;
    
    char ptr_buff[UCI_PTR_STR_BUFF_SIZE];
    
    u_ctx = uci_alloc_context();
    if(!u_ctx) {
        log_message(LOG_ERROR, "Could not allocate UCI context for changing WiFi SSID\r\n");
        return false;
    }
    
    int result = snprintf(ptr_buff, UCI_PTR_STR_BUFF_SIZE, UCI_WIFI_SET_DIS, network, enabled ? "0" : "1");
    if(result < 0 ) {
        log_message(LOG_ERROR, "Encoding error in snprintf while setting WiFi disabled state");
        goto error;
    }
    if (result >= UCI_PTR_STR_BUFF_SIZE) {
        log_message(LOG_ERROR, "'network' and 'ssid' to long for buffer while setting WiFi disabled state\r\n");
        goto error;
    }
    
    /* Set and commit the new state */
    if(uci_parse_ptr(u_ctx, &u_ptr, ptr_buff) != UCI_OK){
        log_message(LOG_ERROR, "Could not parse UCI pointer while setting WiFi disabled state\r\n");   
        log_message(LOG_DEBUG, "UCI pointer string: '%s'\r\n", ptr_buff);
        goto error;
    }
    
    if(uci_set(u_ctx, &u_ptr) != UCI_OK) {
        log_message(LOG_ERROR, "Could not UCI set new state for WiFi network: %s\r\n", network);
        goto error;    
    }
    
    if(uci_commit(u_ctx, &u_ptr.p, false) != UCI_OK) {
        log_message(LOG_ERROR, "Could not commit UCI changes while setting WiFi state for: %s\r\n", network);
        goto error; 
    }
    
    /* Free UCI context */
    uci_free_context(u_ctx);
    return true;
    
error:
    uci_get_errorstr(u_ctx, &err_str, "UCI error");
    log_message(LOG_ERROR, "%s\r\n", err_str);
    free(err_str);
    uci_free_context(u_ctx);
    return false;       
}

/**
 * Connect the board to a WiFi network as a client.
 * @param ssid the SSID to connect to.
 * @param key the type of security to use.
 * @param password the password from the WiFi network.
 * @param username the username, this is necessary for radius.
 * @return true on success.
 */
bool wifi_client_connect(const char* ssid, const int security, const char* key, const char* username)
{
    struct uci_context *u_ctx = NULL;
    struct uci_ptr u_ptr;
    char* err_str;
    int result;
    
    char ptr_buff[UCI_PTR_STR_BUFF_SIZE];
    
    u_ctx = uci_alloc_context();
    if(!u_ctx) {
        log_message(LOG_ERROR, "Could not allocate UCI context for connecting to WiFi network\r\n");
        return false;
    }
    
    /* Set the client network SSID */
    result = snprintf(ptr_buff, UCI_PTR_STR_BUFF_SIZE, UCI_WIFI_SET_SSID, "client", ssid);
    if(result < 0 || result >= UCI_PTR_STR_BUFF_SIZE) {
        log_message(LOG_ERROR, "Encoding error in snprintf while setting WiFi SSID");
        goto error;
    }
    if(uci_parse_ptr(u_ctx, &u_ptr, ptr_buff) != UCI_OK){
        log_message(LOG_ERROR, "Could not parse UCI pointer while setting WiFi SSID\r\n");   
        log_message(LOG_DEBUG, "UCI pointer string: '%s'\r\n", ptr_buff);
        goto error;
    }
    if(uci_set(u_ctx, &u_ptr) != UCI_OK) {
        log_message(LOG_ERROR, "Could not UCI set new SSID for client WiFi network.\r\n");
        goto error;    
    }
    
    /* Set up client network encryption */
    switch(security) {
        case NL_WIFI_SECURITY_NONE:
            result = snprintf(ptr_buff, UCI_PTR_STR_BUFF_SIZE, UCI_WIFI_SET_ENC, UCI_CLIENT_WIFI, "none");
            break;
        case NL_WIFI_SECURITY_WEP:
            result = snprintf(ptr_buff, UCI_PTR_STR_BUFF_SIZE, UCI_WIFI_SET_ENC, UCI_CLIENT_WIFI, "wep");
            break;
        case NL_WIFI_SECURITY_WPA_P:
        case NL_WIFI_SECURITY_WPA_E:
            result = snprintf(ptr_buff, UCI_PTR_STR_BUFF_SIZE, UCI_WIFI_SET_ENC, UCI_CLIENT_WIFI, "psk");
            break;
        case NL_WIFI_SECURITY_WPA2_P:
        case NL_WIFI_SECURITY_WPA2_E:
            result = snprintf(ptr_buff, UCI_PTR_STR_BUFF_SIZE, UCI_WIFI_SET_ENC, UCI_CLIENT_WIFI, "psk2");
            break;
    }
    if(result < 0 || result >= UCI_PTR_STR_BUFF_SIZE) {
        log_message(LOG_ERROR, "Encoding error in snprintf while setting client encryption\r\n");
        goto error;
    }
    if(uci_parse_ptr(u_ctx, &u_ptr, ptr_buff) != UCI_OK){
        log_message(LOG_ERROR, "Could not parse UCI pointer while setting client encryption\r\n");   
        log_message(LOG_DEBUG, "UCI pointer string: '%s'\r\n", ptr_buff);
        goto error;
    }
    if(uci_set(u_ctx, &u_ptr) != UCI_OK) {
        log_message(LOG_ERROR, "Could not UCI set client WiFi encryption\r\n");
        goto error;    
    }
    
    /* Set up client network key */
    switch(security) {
        case NL_WIFI_SECURITY_NONE:
            result = snprintf(ptr_buff, UCI_PTR_STR_BUFF_SIZE, UCI_WIFI_SET_KEY, UCI_CLIENT_WIFI, "");
            break;
        case NL_WIFI_SECURITY_WEP:
            log_message(LOG_ERROR, "WEP is not yet implemented\n\r");
            break;
        case NL_WIFI_SECURITY_WPA_P:
        case NL_WIFI_SECURITY_WPA2_P:
            result = snprintf(ptr_buff, UCI_PTR_STR_BUFF_SIZE, UCI_WIFI_SET_KEY, UCI_CLIENT_WIFI, key);
            break;
        case NL_WIFI_SECURITY_WPA_E:
        case NL_WIFI_SECURITY_WPA2_E:
            log_message(LOG_ERROR, "WPA-Enterprise is not yet implemented\n\r");
            goto error;
            break;
    }
    if(result < 0 || result >= UCI_PTR_STR_BUFF_SIZE) {
        log_message(LOG_ERROR, "Encoding error in snprintf while setting client key\r\n");
        goto error;
    }
    if(uci_parse_ptr(u_ctx, &u_ptr, ptr_buff) != UCI_OK){
        log_message(LOG_ERROR, "Could not parse UCI pointer while setting client key\r\n");   
        log_message(LOG_DEBUG, "UCI pointer string: '%s'\r\n", ptr_buff);
        goto error;
    }
    if(uci_set(u_ctx, &u_ptr) != UCI_OK) {
        log_message(LOG_ERROR, "Could not UCI set client WiFi key\r\n");
        goto error;    
    }
    
    /* Enable client network */
    result = snprintf(ptr_buff, UCI_PTR_STR_BUFF_SIZE, UCI_WIFI_SET_DIS, UCI_CLIENT_WIFI, "0");
    if(result < 0 || result >= UCI_PTR_STR_BUFF_SIZE) {
        log_message(LOG_ERROR, "Encoding error in snprintf while enabling client mode\r\n");
        goto error;
    }
    if(uci_parse_ptr(u_ctx, &u_ptr, ptr_buff) != UCI_OK){
        log_message(LOG_ERROR, "Could not parse UCI pointer while enabling client mode\r\n");   
        log_message(LOG_DEBUG, "UCI pointer string: '%s'\r\n", ptr_buff);
        goto error;
    }
    if(uci_set(u_ctx, &u_ptr) != UCI_OK) {
        log_message(LOG_ERROR, "Could not UCI enable client WiFi network.\r\n");
        goto error;    
    }
    
    /* Commit the WiFi changes */ 
    if(uci_commit(u_ctx, &u_ptr.p, false) != UCI_OK) {
        log_message(LOG_ERROR, "Could not commit UCI changes while setting WiFi SSID:\r\n");
        goto error; 
    }
    
    /* Mark this action as a new WiFi connection attempt */
    new_wifi_attempt = true;
    clear_new_wifi_command = false;
    
    /* Free UCI context */
    uci_free_context(u_ctx);
    return true;
    
error:
    uci_get_errorstr(u_ctx, &err_str, "UCI error");
    log_message(LOG_ERROR, "%s\r\n", err_str);
    free(err_str);
    uci_free_context(u_ctx);
    return false;   
}

/**
 * Reload networking and WiFi configuration by restarting WiFi hardware
 * @return true on success, false on failure
 */
bool wifi_restart()
{
    if(system("wifi") == -1){
        log_message(LOG_ERROR, "WiFi restart could not successfully reload network\r\n");
        return false;
    }
    return true;
}

/**
 * Returns true when the client network is enabled.
 * @return true when the network section is enabled.
 */
bool wifi_client_iface_enabled()
{
    bool enabled;
    
    pthread_mutex_lock(&(wifi_adapt_info.lock));
    enabled = wifi_adapt_info.client.enabled;
    pthread_mutex_unlock(&(wifi_adapt_info.lock));
    
    return enabled;      
}

/**
 * Returns true when the configuration network is enabled.
 * @return true when the network section is enabled.
 */
bool wifi_cfg_iface_enabled()
{
    bool enabled;
    
    pthread_mutex_lock(&(wifi_adapt_info.lock));
    enabled = wifi_adapt_info.cfg.enabled;
    pthread_mutex_unlock(&(wifi_adapt_info.lock));
    
    return enabled;      
}

/**
 * Returns true when the client network is enabled.
 * @return true when the client network is enabled.
 */
bool wifi_client_network_connected()
{
    bool connected;
    
    pthread_mutex_lock(&(wifi_adapt_info.lock));
    connected = wifi_adapt_info.client.connected;
    pthread_mutex_unlock(&(wifi_adapt_info.lock));
    
    return connected;
}
