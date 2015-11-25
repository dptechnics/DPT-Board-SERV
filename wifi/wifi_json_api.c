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
 * File:   wifi_json_api.c
 * Created on March 14, 2015, 10:04 PM
 */

#include <stdio.h>
#include <stdio.h>
#include <json-c/json.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#include "../uhttpd.h"
#include "../logger.h"
#include "../api.h"
#include "../helper.h"
#include "wifi.h"
#include "wifi_json_api.h"
#include "nl.h"
#include "iwinfo.h"

static const char* networks[3] = {
    [1] = UCI_CFG_WIFI,
    [2] = UCI_CLIENT_WIFI
};

/**
 * Route all get requests concerning the wifi module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* wifi_get_router(struct client *cl, char *request)
{
    if (helper_str_startswith(request, "scan", 0)) 
    {
        return wifi_get_scan(cl, request);
    }
    else if(helper_str_startswith(request, "requestscan", 0))
    {
        return wifi_get_scantrigger(cl, request);
    }
    else if (helper_str_startswith(request, "info", 0))
    {
        return wifi_get_info(cl, request);
    }
    else
    {
        log_message(LOG_WARNING, "WiFi API got unknown GET request '%s'\r\n", request);
        return NULL;
    }
}

/**
 * Route all post requests concerning the wifi module.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return the result of the called function.
 */
json_object* wifi_post_router(struct client *cl, char *request)
{
    if (helper_str_startswith(request, "setssid", 0)) 
    {
        return wifi_post_ssid_change(cl, request);
    }
    else if(helper_str_startswith(request, "setstate", 0)) 
    {
        return wifi_post_state_change(cl, request);
    }
    else if(helper_str_startswith(request, "setsimplesettings", 0)) 
    {
        return wifi_post_simplesettings_change(cl, request);
    }
    else if (helper_str_startswith(request, "connect", 0))
    {
        return wifi_post_connect(cl, request);
    }
    else
    {
        log_message(LOG_WARNING, "WiFi API got unknown POST request '%s'\r\n", request);
        return NULL;
    }
}

/**
 * Scan for available wifi networks and return information 
 * @cl the client who made the request
 * @request the request part of the url
 * @return the wifi information
 */
json_object* wifi_get_scan(struct client *cl, char *request)
{
    json_object *result = json_object_new_object();
    
    /* Lock the network list for reading */
    pthread_mutex_lock(&(wifi_list.lock));
    
    /* Check if the list is editing or not */
    if(wifi_list.editing) {
        json_object *j_result = json_object_new_string("busy");
        json_object_object_add(result, "result", j_result);
    } else {
        json_object *j_result = json_object_new_string("done");
        json_object_object_add(result, "result", j_result);
        json_object *j_array = json_object_new_array();
        
        /* Add all networks to the json object */
        struct nl_wifi_network *net = wifi_list.list;
        while(net != NULL) {
            json_object *network = json_object_new_object();
            
            json_object *ssid = json_object_new_string(net->ssid);
            json_object *dbsig = json_object_new_int(net->signal);
            json_object *dbqual = json_object_new_int(net->quality);
            json_object *secenabled = json_object_new_boolean(net->security != NL_WIFI_SECURITY_NONE);
            json_object *sectype = json_object_new_int(net->security);
            json_object *secreadable = json_object_new_string(nl_sec_map[net->security]);

            
            /* Add elements to network and network to array */
            json_object_object_add(network, "ssid", ssid);
            //json_object_object_add(network, "channel", channel);
            json_object_object_add(network, "signal", dbsig);
            json_object_object_add(network, "quality", dbqual);
            json_object_object_add(network, "secured", secenabled);
            json_object_object_add(network, "sec_type", sectype);
            json_object_object_add(network, "sec_readable", secreadable);
            
            net = net->next;
            json_object_array_add(j_array, network);
        }

        json_object_object_add(result, "networks", j_array);
    }
    
    /* Release the network list */
    pthread_mutex_unlock(&(wifi_list.lock));   
    
    cl->http_status = r_ok;
    return result;
}

/**
 * Entry point for the WiFi network scan thread
 * @param args the thread arguments
 */
static void* scanning_thread(void* args)
{
    wifi_scan_networks();
    log_message(LOG_DEBUG, "Completed WiFi scan request\r\n");
    pthread_exit(NULL);
}

/**
 * Request a WiFi scan, this will go async, and return immediately. 
 * @param cl the client who made the request. 
 * @param request the request part of the url.
 * @return true on success, false on error.
 */
json_object* wifi_get_scantrigger(struct client *cl, char *request)
{
    bool result = false;
    pthread_t thread;
    if(pthread_create(&thread, NULL, scanning_thread, NULL) != 0) {
        log_message(LOG_ERROR, "Could not create WiFi scanning thread\r\n");
    } else {
        result = true;
        pthread_detach(thread);
    }
    
    json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "result", json_object_new_int(result ? WIFI_RESULT_OK : WIFI_ERROR_UNKNOWN_ERROR));
    
    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
}

/**
 * Add information from a wifi adapter to a json object.
 * @param info the wifi adapter info to add to the json object.
 * @param j_wifi the json object to add the data to.
 */
static void _add_adapterinfo_to_json(struct wifi_adapter *info, json_object* j_wifi)
{
    
        /* Add SSID */
    json_object *j_ssid = json_object_new_string(info->ssid);
    json_object_object_add(j_wifi, "SSID", j_ssid);

    /* Add channel */
    json_object *j_channel = json_object_new_int(info->channel);
    json_object_object_add(j_wifi, "channel", j_channel);

    /* Add signal */
    json_object *j_signal = json_object_new_int(info->signal);
    json_object_object_add(j_wifi, "signal", j_signal);

    /* Add quality */
    json_object *j_quality = json_object_new_int(info->quality);
    json_object_object_add(j_wifi, "quality", j_quality);

    /* Add mode */
    json_object *j_mode = json_object_new_string(IWINFO_OPMODE_NAMES[info->mode]);
    json_object_object_add(j_wifi, "mode", j_mode);
    
    /* Add enabled */
    json_object *j_enabled = json_object_new_boolean(info->enabled);
    json_object_object_add(j_wifi, "enabled", j_enabled);
    
    /* Add connected */
    json_object *j_connected = json_object_new_boolean(info->connected);
    json_object_object_add(j_wifi, "connected", j_connected);
}

/**
 * Get all information about the wireless interfaces
 * currently active. 
 * @param cl the client who made the request. 
 * @param request the request part of the url. 
 * @return the wifi information
 */
json_object* wifi_get_info(struct client *cl, char *request)
{ 
    
    /* Make the wireless interface array */
    json_object *j_array = json_object_new_array(); 
    
    /* Acquire lock on WiFi adapter information */
    pthread_mutex_lock(&(wifi_adapt_info.lock));
    
    /* Add configuration network */
    json_object *j_wifi = json_object_new_object();
    json_object_object_add(j_wifi, "network", json_object_new_string("config"));
    _add_adapterinfo_to_json(&(wifi_adapt_info.cfg), j_wifi);
    json_object_array_add(j_array, j_wifi);
    
    /* Add client network */
    j_wifi = json_object_new_object();
    json_object_object_add(j_wifi, "network", json_object_new_string("client"));
    _add_adapterinfo_to_json(&(wifi_adapt_info.client), j_wifi);
    json_object_array_add(j_array, j_wifi);
    
    pthread_mutex_unlock(&(wifi_adapt_info.lock));
    
    /* Return status ok */
    cl->http_status = r_ok;
    return j_array;
}

/**
 * Post a new WiFi client configuration.
 * @cl the client who made the request
 * @request the request part of the url
 * @return json object marking success or not
 */
json_object* wifi_post_connect(struct client *cl, char *request)
{
    /* Parse JSON post data */
    json_object *in_obj = json_tokener_parse(cl->postdata);
    
    json_object *j_ssid = NULL;
    json_object *j_security = NULL;
    json_object *j_username = NULL;
    json_object *j_password = NULL;
    if(!json_object_object_get_ex(in_obj, "ssid", &j_ssid) ||
       !json_object_object_get_ex(in_obj, "security", &j_security) ||
       !json_object_object_get_ex(in_obj, "username", &j_username) ||
       !json_object_object_get_ex(in_obj, "password", &j_password)) 
    {
        if(json_object_put(in_obj) != 1) {
            log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
        }
        return NULL;
    }
    
    const char* ssid = json_object_get_string(j_ssid);
    const int security = json_object_get_int(j_security);
    const char* username = json_object_get_string(j_username);
    const char* password = json_object_get_string(j_password);
    
    bool result = false;
    
    switch(security) {
        case NL_WIFI_SECURITY_NONE:
            result = wifi_client_connect(ssid, security, "", "");
        case NL_WIFI_SECURITY_WPA_P:
        case NL_WIFI_SECURITY_WPA2_P:
            result = wifi_client_connect(ssid, security, password, "");
        case NL_WIFI_SECURITY_WPA_E:
        case NL_WIFI_SECURITY_WPA2_E:
            result = wifi_client_connect(ssid, security, password, username);
    }
    
    /* Create info object */
    json_object *jobj = json_object_new_object();
    json_object *j_status = json_object_new_boolean(result);
    json_object_object_add(jobj, "status", j_status);

    /* Release parsed json object */
    if(json_object_put(in_obj) != 1) {
        log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
    }
    
    if(!wifi_restart()) {
        log_message(LOG_ERROR, "Could not succesfully restart WiFi interfaces\r\n");
    }
    
    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
}

/**
 * Change the ssid of a given network
 * @param cl the client who made the request
 * @param request the request part of the url
 * @return json object marking success or not
 */
json_object* wifi_post_ssid_change(struct client *cl, char *request) 
{
    /* Parse JSON post data */
    json_object *in_obj = json_tokener_parse(cl->postdata);
    
    json_object *j_network = NULL;
    json_object *j_ssid = NULL;
    if(!json_object_object_get_ex(in_obj, "network", &j_network) ||
       !json_object_object_get_ex(in_obj, "ssid", &j_ssid)) 
    {
        if(json_object_put(in_obj) != 1) {
            log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
        }
        return NULL;
    }
    
    const char* network = json_object_get_string(j_network);
    const char* ssid = json_object_get_string(j_ssid);
    
    /* Set the new WiFi SSID */
    bool success = wifi_set_ssid(network, ssid);
    
    /* Create info object */
    json_object *jobj = json_object_new_object();
    json_object *j_status = json_object_new_boolean(success);
    json_object *jo_ssid = json_object_new_string(ssid);
    json_object_object_add(jobj, "status", j_status);
    json_object_object_add(jobj, "ssid", jo_ssid);

    /* Release parsed json object */
    if(json_object_put(in_obj) != 1) {
        log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
    }
    
    if(!wifi_restart()) {
        log_message(LOG_ERROR, "Could not succesfully restart WiFi interfaces\r\n");
    }
    
    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
}

/**
 * Change the state of a network.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return json object marking success or not.
 */
json_object* wifi_post_state_change(struct client *cl, char *request)
{
    /* Parse JSON post data */
    json_object *in_obj = json_tokener_parse(cl->postdata);
    
    json_object *j_network = NULL;
    json_object *j_state = NULL;
    if(!json_object_object_get_ex(in_obj, "network", &j_network) ||
       !json_object_object_get_ex(in_obj, "state", &j_state)) 
    {
        if(json_object_put(in_obj) != 1) {
            log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
        }
        return NULL;
    }
    
    int network = json_object_get_int(j_network);
    bool state = json_object_get_boolean(j_state);
    
    /* Create info object */
    json_object *jobj = json_object_new_object();
    
    /* Check if the network is in range */
    if(network != WIFI_JSON_BOARD_NETWORK && network != WIFI_JSON_CLIENT_NETWORK) {
        json_object_object_add(jobj, "result", json_object_new_int(WIFI_ERROR_BAD_NETWORK_DESCRIPTOR));
        json_object_object_add(jobj, "message", json_object_new_string("Bad network descriptor given."));
        cl->http_status = r_bad_req;
        goto error;
    }
    
    /* Set the new WiFi state */
    if(!wifi_set_state(networks[network], state)) {
        json_object_object_add(jobj, "result", json_object_new_int(WIFI_ERROR_UNKNOWN_ERROR));
        json_object_object_add(jobj, "message", json_object_new_string("Unknown error while setting network state"));
        cl->http_status = r_error;
        goto error;
    }
    
    /* Restart the WiFi adapter */
    if(!wifi_restart()) {
        json_object_object_add(jobj, "result", json_object_new_int(WIFI_ERROR_NO_RESTART));
        json_object_object_add(jobj, "message", json_object_new_string("Could not restart the WiFi adapter"));
        cl->http_status = r_error;
        goto error;
    }
    
    /* Release parsed json object */
    if(json_object_put(in_obj) != 1) {
        log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
    }
    
    /* Create success object */
    json_object_object_add(jobj, "result", json_object_new_int(WIFI_RESULT_OK));
    json_object_object_add(jobj, "state", json_object_new_boolean(state));
    
    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
    
error:
    /* Release parsed json object */
    if(json_object_put(in_obj) != 1) {
        log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
    }
    return jobj;  
}

/**
 * Change the state and ssid of a network.
 * @param cl the client who made the request.
 * @param request the request part of the url.
 * @return json object marking success or not.
 */
json_object* wifi_post_simplesettings_change(struct client *cl, char *request)
{
    /* Parse JSON post data */
    json_object *in_obj = json_tokener_parse(cl->postdata);
    
    json_object *j_network = NULL;
    json_object *j_ssid = NULL;
    json_object *j_state = NULL;
    if(!json_object_object_get_ex(in_obj, "network", &j_network) ||
       !json_object_object_get_ex(in_obj, "ssid", &j_ssid) ||
       !json_object_object_get_ex(in_obj, "state", &j_state)) 
    {
        if(json_object_put(in_obj) != 1) {
            log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
        }
        return NULL;
    }
    
    int network = json_object_get_int(j_network);
    const char* ssid = json_object_get_string(j_ssid);
    bool state = json_object_get_boolean(j_state);
    
    /* Create info object */
    json_object *jobj = json_object_new_object();
    
    /* Check if the network is in range */
    if(network != WIFI_JSON_BOARD_NETWORK && network != WIFI_JSON_CLIENT_NETWORK) {
        json_object_object_add(jobj, "result", json_object_new_int(WIFI_ERROR_BAD_NETWORK_DESCRIPTOR));
        json_object_object_add(jobj, "message", json_object_new_string("Bad network descriptor given."));
        cl->http_status = r_bad_req;
        goto error;
    }
    
    /* Check if the SSID is no longer than 36 chars (max WiFi SSID length) */
    if(strlen(ssid) > 36) {
        json_object_object_add(jobj, "result", json_object_new_int(WIFI_ERROR_SSID_TO_LONG));
        json_object_object_add(jobj, "message", json_object_new_string("SSID is longer than 36 characters"));
        cl->http_status = r_bad_req;
        goto error;
    }
    
    /* Set the new WiFi SSID */
    if(!wifi_set_ssid(networks[network], ssid)) {
        json_object_object_add(jobj, "result", json_object_new_int(WIFI_ERROR_UNKNOWN_ERROR));
        json_object_object_add(jobj, "message", json_object_new_string("Unknown error while setting SSID"));
        cl->http_status = r_error;
        goto error;
    }
    
    /* Set the new WiFi state */
    if(!wifi_set_state(networks[network], state)) {
        json_object_object_add(jobj, "result", json_object_new_int(WIFI_ERROR_UNKNOWN_ERROR));
        json_object_object_add(jobj, "message", json_object_new_string("Unknown error while setting network state"));
        cl->http_status = r_error;
        goto error;
    }
    
    /* Restart the WiFi adapter */
    if(!wifi_restart()) {
        json_object_object_add(jobj, "result", json_object_new_int(WIFI_ERROR_NO_RESTART));
        json_object_object_add(jobj, "message", json_object_new_string("Could not restart the WiFi adapter"));
        cl->http_status = r_error;
        goto error;
    }
    
    /* Release parsed json object */
    if(json_object_put(in_obj) != 1) {
        log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
    }
    
    /* Create success object */
    json_object_object_add(jobj, "result", json_object_new_int(WIFI_RESULT_OK));
    json_object_object_add(jobj, "ssid", json_object_new_string(ssid));
    json_object_object_add(jobj, "state", json_object_new_boolean(state));
    
    /* Return status ok */
    cl->http_status = r_ok;
    return jobj;
    
error:
    /* Release parsed json object */
    if(json_object_put(in_obj) != 1) {
        log_message(LOG_WARNING, "[memleak] Memory of parsed JSON object is not freed\r\n");
    }
    return jobj;    
}
