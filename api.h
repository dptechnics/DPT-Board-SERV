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
 * File:   api.h
 * Created on August 6, 2014, 5:28 PM
 */

#ifndef API_H
#define API_H

#include <sys/types.h>

#include "uhttpd.h"
#include "config.h"

/**
 * Struct mapping a string to a function pointer
 */
struct f_entry {
	const char name[API_CALL_MAX_LEN];
        size_t url_offset;
	void* function;
};

/**
 * Handle api requests
 * @cl the client who sent the request
 * @url the request URL
 */
void api_handle_request(struct client *cl, char *url);

/**
 * Get the API handler structure if any. 
 * @name the request url.
 * @offset the offset at which the method starts in the url.
 * @table the function lookup table, must be in lexical order.
 * @table_size the size of the table.
 * @return a handler struct if any, NULL otherways.
 */
const struct f_entry* api_get_function(char* url, size_t offset, const struct f_entry* table, size_t table_size);

#endif
