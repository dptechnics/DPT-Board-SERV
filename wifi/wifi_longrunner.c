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
 * File:   wifi_longrunner.c
 * Created on May 15, 2015, 8:39 AM
 */

#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "../longrunner.h"
#include "../logger.h"
#include "wifi_longrunner.h"
#include "wifi.h"
#include "wifi_json_api.h"

#define WIFI_ERR_TIME   30              /* Number of seconds the client WiFi can be down */
#define WIFI_RETRY_TIME 180             /* Number of seconds before WiFi will retry to connect */ 

bool bad_connection;                    /* True when connection is bad */
time_t bad_conn_start;                  /* Start of bad WiFi connection time, only valid when bad_connection is true */
bool new_wifi_attempt = false;          /* External bool from wifi.h */
bool clear_new_wifi_command = false;    /* External bool from wifi.h */  
bool marked_for_restart = false;        /* If try the WiFi connection will try to recover after timeout */

/**
 * The WiFi longrunner initializer
 */
void wifi_longrunner_init(void)
{
    longrunner_add(wifi_longrunner_entrypoint, 2500);
    
    bad_connection = false;
    new_wifi_attempt = false;
    marked_for_restart = false;
    clear_new_wifi_command = false;
    
    /* Scan network once in the beginning */
    wifi_scan_networks();
    log_message(LOG_INFO, "WiFi longrunner initialised\r\n");
}

/**
 * The WiFi longrunner entry point. 
 */
void wifi_longrunner_entrypoint(void)
{
    /* Check if the WiFi must be restarted as a cause of bad password */
    wifi_make_summary();

    /* Checking not necessary if client WiFi is not enabled*/
    if(wifi_client_iface_enabled()) {
        if(!wifi_client_network_connected()) {
            if(bad_connection) {
                /* Check how long this connection is bad */
                double diff = difftime(time(NULL), bad_conn_start);
                if(diff > WIFI_ERR_TIME) {
                    /* Turning off WiFi because error time passed */
                    wifi_set_state(UCI_CLIENT_WIFI, false);
                    wifi_restart();
                    log_message(LOG_INFO, "Disabled client WiFi connection because of bad connection\r\n");
                    
                    if(!new_wifi_attempt) {
                        /* This is not a new client WiFi connection attempt, try to restore connection after timeout */
                        marked_for_restart = true;
                        bad_conn_start = time(NULL);
                        log_message(LOG_INFO, "Marked disabled WiFi connection for restart after %d seconds\r\n", WIFI_RETRY_TIME);
                    } else {
                        marked_for_restart = false;
                    }
                } else {
                    log_message(LOG_DEBUG, "WiFi has bad connection and will be disabled in %.2f seconds\r\n", WIFI_ERR_TIME - diff);
                }
            } else {
                /* Mark this connection as bad */
                bad_conn_start = time(NULL);
                bad_connection = true;
                clear_new_wifi_command = true;
                log_message(LOG_DEBUG, "Marked client WiFi connection as bad\r\n");
            }
        } else {
            log_message(LOG_DEBUG, "Client WiFi network is connected\r\n");
            bad_connection = false;
            
            if(clear_new_wifi_command) {
                new_wifi_attempt = false;
            }
        }
    } else {
        log_message(LOG_INFO, "Client WiFi interface is disabled\r\n");
        bad_connection = false;
        
        if(marked_for_restart) {
            double diff = difftime(time(NULL), bad_conn_start);
            if(diff > WIFI_RETRY_TIME) {
                wifi_set_state(UCI_CLIENT_WIFI, true);
                wifi_restart();
                log_message(LOG_INFO, "Attempted to reconnect client WiFi connection\r\n");
            } else {
                log_message(LOG_DEBUG, "WiFi marked for restart in %.2f seconds\r\n", WIFI_RETRY_TIME - diff);
            }
        }
    }
}
