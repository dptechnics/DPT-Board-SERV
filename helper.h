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
 * File:   helper.h
 * Created on February 1, 2015, 4:08 PM
 */

#ifndef HELPER_H
#define	HELPER_H

#include <stddef.h>
#include <stdbool.h>

/**
 * Structure used by unserialize
 */
struct chararray {
    char** array;
    size_t len;
};

/**
 * Serialize a string array
 * @param array the character array
 * @param len the length of the character array
 * @return a string, must be freed by the user, containing the serialized array. 
 */
char* helper_serialize_str_array(char** array, size_t len);

/**
 * Unserialize a string previously serialized with _serialize_str_array
 * @param str the serialized string array
 * @return structure containing a string array. This character array should
 * be freed by the user.  
 */
struct chararray* helper_unserialize_str_array(char* str);

/**
 * Free a character array returned by helper functions
 * @param chr_array the character array to free
 */
void helper_free_char_array(struct chararray *chr_array);

/**
 * Returns true if the haystack starts with the needle string.
 * @param haystack the string to search in. 
 * @param needle the string with which the haystack must start. 
 * @param offset the offset at which we need to search in the haystack. 
 * @return false when the haystack does not start with the needle. 
 */
bool helper_str_startswith(const char* haystack, const char* needle, size_t offset);

#endif

