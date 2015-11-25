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
 * File:   helper.c
 * Created on February 1, 2015, 4:08 PM
 */

#include <string.h>
#include <stdlib.h>

#include "helper.h"
#include "logger.h"

/* Use unit separator (31) as delimiter*/
#define SER_DELIM   "\31"

/**
 * Serialize a string array
 * @param array the character array
 * @param len the length of the character array
 * @return a string, must be freed by the user, containing the serialized array. 
 */
char* helper_serialize_str_array(char** array, size_t len)
{
    char* serialized = NULL;
    size_t i, l, s_len;
    
    /* Append first string */
    l = strlen(array[0]);
    s_len = l + 2;
    serialized = (char*) realloc(serialized, s_len);
    strcpy(serialized, array[0]);
    strcat(serialized, SER_DELIM);
    
    /* Append every string and delimiter */
    for(i = 1; i < len; ++i) {
        l = strlen(array[i]);
        s_len += l + 1;
        serialized = (char*) realloc(serialized, s_len);
        if(serialized == NULL) {
            log_message(LOG_ERROR, "Ran out of memory when serializing string\r\n");
            return NULL;
        }
        
        strcat(serialized, array[i]);
        strcat(serialized, SER_DELIM);
    }
    
    /* Null terminate serialized string */
    serialized[s_len - 2] = '\0';
    return serialized;
}

/**
 * Unserialize a string previously serialized with _serialize_str_array
 * @param str the serialized string array
 * @return structure containing a string array. This character array should
 * be freed by the user.  
 */
struct chararray* helper_unserialize_str_array(char* str)
{
    struct chararray* chr_array = (struct chararray*) malloc(sizeof(struct chararray));
    chr_array->len = 0;
    char *to_save;
    char *tmp = str;
    size_t i;
    
    /* Return empty array on empty string*/
    if(strlen(str) == 0) {
        return chr_array;
    }
    
    /* Count the number of elements in the serialized string*/
    while(*tmp) {
        if(SER_DELIM[0] == *tmp)
            ++(chr_array->len);
        ++tmp;
    }
    ++(chr_array->len);
    chr_array->array = (char**) malloc(chr_array->len * sizeof(char*));
    
    char* pch = strtok(str, SER_DELIM);
    for(i = 0; i < chr_array->len; ++i){
        to_save = (char*) malloc((strlen(pch) + 1) * sizeof(char));
        strcpy(to_save, pch);
        chr_array->array[i] = to_save;
        pch = strtok(NULL, SER_DELIM);
    }
    return chr_array;
}

/**
 * Free a character array returned by helper functions
 * @param chr_array the character array to free
 */
void helper_free_char_array(struct chararray *chr_array)
{
    size_t i;
    for(i = 0; i < chr_array->len; ++i) {
        free(chr_array->array[i]);
    }
    free(chr_array);
}

/**
 * Returns true if the haystack starts with the needle string.
 * @param haystack the string to search in. 
 * @param needle the string with which the haystack must start. 
 * @param offset the offset at which we need to search in the haystack. 
 * @return false when the haystack does not start with the needle. 
 */
bool helper_str_startswith(const char* haystack, const char* needle, size_t offset)
{
    haystack += offset;
    
    while(*needle) {
        if(*needle != *haystack) {
            return false;
        }
        ++needle;
        ++haystack;
    }
    
    return true;
}
