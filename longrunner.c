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
 * File:   longrunner.c
 * Created on May 14, 2015, 9:22 PM
 */

#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

#include "longrunner.h"
#include "logger.h"

/**
 * Longrunner methods list
 */
longrunner_method* l_list;

/**
 * The last longrunner list item
 */
longrunner_method* l_anchor;

/**
 * Initialize the longrunners 
 */
void longrunner_init()
{
    l_list = (longrunner_method *) calloc(1, sizeof(longrunner_method));
    l_anchor = l_list;
}

/**
 * Add a longrunner entrypoint to the longrunner engine. 
 * @param function the function to execute. 
 * @param timeout the timeout for repetition after it's done. 
 */
void longrunner_add(void* function, uint32_t timeout)
{
    longrunner_method* new_method =  (longrunner_method *) calloc(1, sizeof(longrunner_method));
    new_method->function = function;
    new_method->timeout_ms = timeout;
    l_anchor->next = new_method;
    l_anchor = new_method;
}

/**
 * Start the longrunner threads
 */
void longrunner_start()
{
    int i = 1;
    longrunner_method* l_method = l_list->next;
    
    while(l_method != NULL) {
        log_message(LOG_INFO, "Starting longrunner thread %d\r\n", i++);
        pthread_t thread;
        if(pthread_create(&thread, NULL, longrunner_thread, (void*) l_method) != 0) {
            log_message(LOG_ERROR, "Could not create longrunner thread\r\n");
        }
        
        pthread_detach(thread);
        
        l_method = l_method->next;
    }
}

/**
 * A longrunner thread entry point
 * @param args the longrunner_method 
 */
void* longrunner_thread(void* args)
{
    longrunner_method* l_method = (longrunner_method*) args;
    void* (*function)(void) = NULL;
    function = l_method->function;
    
    /* Ensure this is not a null pointer function */
    if(function == NULL) {
        log_message(LOG_WARNING, "Longrunner function is NULL\r\n");
        return NULL;
    }  
    
    while(true) {
        /* Run the longrunner task */
        function();
        
        /* Wait for x ms after execution */
        usleep(l_method->timeout_ms*1000);
    }
    
    pthread_exit(NULL);
}
