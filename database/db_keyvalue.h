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
 * File:   db_keyvalue.h
 * Created on April 13, 2015, 7:21 PM
 */

#ifndef DB_KEYVALUE_H
#define	DB_KEYVALUE_H

#include <stdbool.h>

/**
 * Initialize keyvalue database
 * @db the database to work with
 * @return true on success, false on error
 */
int dao_keyvalue_init(sqlite3 *db);

/*
 * Put a new integer in the configuration table
 * @key the configuration key
 * @value the value to wich to key points
 */
int dao_keyvalue_put_int(const char* key, int value);

/*
 * Put a new text value in the configuration table
 * @key the configuration key
 * @value the value to which to key points
 */
int dao_keyvalue_put_text(const char* key, const char* value);

/*
 * Update configuration integer value
 * @key the configuration key
 * @value the value to which to key points
 */
int dao_keyvalue_edit_int(const char* key, int value);

/*
 * Update configuration text value
 * @key the configuration key
 * @value the value to which to key points
 */
int dao_keyvalue_edit_text(const char* key, const char* value);

/*
 * Get an integer configuration value
 * @key the configuration key
 * @return the return value, with status, this should be freed after use
 */
db_int* dao_keyvalue_get_int(const char* key);

/*
 * Get a text configuration value
 * @key the configuration key
 * @return the return value, with status, this should be freed after use
 */
db_text* dao_keyvalue_get_text(const char* key);

#endif

