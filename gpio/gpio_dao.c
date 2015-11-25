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
 * File:   db_gpio.c
 * Created on April 13, 2015, 7:21 PM
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>

#include "../config.h"
#include "../logger.h"
#include "../database/database.h"
#include "gpio_dao.h"

/**
 * Initialize GPIO database
 * @db the database to work with
 * @return true on success, false on error
 */
int gpio_dao_init(sqlite3 *db) {
    char* err_msg;
    
    char* sql = "CREATE TABLE gpio ( \
			id  INTEGER PRIMARY KEY	ASC NOT NULL, \
			number INTEGER UNIQUE NOT NULL, \
			name TEXT, \
			pulsetime INTEGER \
			);";

    /* Try to create table */
    if (sqlite3_exec(db, sql, NULL, 0, &err_msg) != SQLITE_OK) {
        /* The table could not be constructed, exit with error */
        log_message(LOG_ERROR, "Could not create 'gpio' table\r\n");
        perror(err_msg);
        sqlite3_free(err_msg);
        return DB_ERR;
    }
    
    return DB_OK;
}
