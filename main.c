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
 * File:   main.c
 * Created on May 9, 2014, 5:28 PM
 */

#define _BSD_SOURCE
#define _GNU_SOURCE
#define _XOPEN_SOURCE	700
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>

#include <libubox/uloop.h>
#include <libubox/usock.h>

#include "listen.h"
#include "main.h"
#include "config.h"
#include "uhttpd.h"
#include "api.h"
#include "database/database.h"
#include "logger.h"
#include "longrunner.h"

#include "wifi/wifi_longrunner.h"

/**
 * The servers main working buffer.
 */
char uh_buf[WORKING_BUFF_SIZE];

/*
 * The UBUS connection context.
 */
struct ubus_context *ubus_ctx = NULL;

/**
 * Main application entry point.
 * @argc the number of command line arguments.
 * @argv the command line arguments.
 */
int main(int argc, char **argv) {
    /* Current file descriptor for /dev/null */
    int cur_fd;

    /* Prevent SIGPIPE errors */
    signal(SIGPIPE, SIG_IGN);

    /* Load default configuration */
    if (!load_configuration()) {
        return EXIT_FAILURE;
    }

    /* fork (if not disabled) */
    if (conf->daemon) {
        switch (fork()) {
            case -1:
                perror("fork()");
                exit(1);

            case 0:
                /* daemon setup */
                if (chdir("/"))
                    perror("chdir()");

                cur_fd = open("/dev/null", O_WRONLY);
                if (cur_fd > 0) {
                    dup2(cur_fd, 0);
                    dup2(cur_fd, 1);
                    dup2(cur_fd, 2);
                }

                break;

            default:
                exit(0);
        }
    }

    /* Initialize database */
    if (dao_create_db() != DB_OK) {
        /* The server can't run without database */
        log_message(LOG_ERROR, "Could not create breakout server database\r\n");
        perror("");
        return EXIT_FAILURE;
    }

    /* Initialize UBUS */
    ubus_ctx = ubus_connect(NULL);
    if (!ubus_ctx) {
        log_message(LOG_ERROR, "Could not connect to UBUS RPC daemon\r\n");
    }
    log_message(LOG_INFO, "Successfully connected to UBUS RPC daemon\r\n");
    
    /* Initialize and start longrunners */
    longrunner_init();
    setup_longrunners();
    longrunner_start();
    
    /* Initialize network event loop */
    uloop_init();

    /* Set up all listener sockets */
    setup_listeners();

    /* Start the network event loop */
    uloop_run();

    return EXIT_SUCCESS;
}

/**
 * Create the server configuration and bind to socket.
 * @return: true if configuration was successful.
 */
bool load_configuration(void) {
    if (!config_parse()) {
        log_message(LOG_ERROR, "Could not parse configuration file\r\n");
        return false;
    }

    /* Bind a non TLS socket to port */
    if (!bind_listener_sockets(NULL, conf->listen_port, false)) {
        log_message(LOG_ERROR, "Could not bind socket to 0.0.0.0:%d\r\n", conf->listen_port);
        return false;
    }

    log_message(LOG_INFO, "Breakout-server started listening on port %s\r\n", conf->listen_port);

    return true;
}

/**
 * Add all longrunner modules to the breakout-server
 */
void setup_longrunners(void) {
    wifi_longrunner_init();
}


