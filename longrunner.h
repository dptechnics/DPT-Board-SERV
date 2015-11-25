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
 * File:   longrunner.h
 * Created on May 14, 2015, 9:22 PM
 */

#ifndef LONGRUNNER_H
#define	LONGRUNNER_H

#include <stdint.h>

/* Linked list of longrunner methods */
typedef struct l_method {
    void* function;             /* The longrunner function to execute */
    uint32_t timeout_ms;        /* Timeout to wait befor re-execution */
    struct l_method* next;    /* The next longrunner function */
} longrunner_method;

/**
 * Initialize the longrunners 
 */
void longrunner_init();

/**
 * Add a longrunner entrypoint to the longrunner engine. 
 * @param function the function to execute. 
 * @param timeout the timeout for repetition after it's done. 
 */
void longrunner_add(void* function, uint32_t timeout);

/**
 * Start the longrunner threads
 */
void longrunner_start();

/**
 * A longrunner thread entry point
 * @param args the longrunner_method 
 */
void* longrunner_thread(void* args);

#endif

