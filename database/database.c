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
 * File:   database.c
 * Created on September 11, 2014, 11:00 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>

#include "database.h"
#include "../config.h"
#include "../logger.h"

/* References to installed DAO modules for initializing*/
#include "../firmware/firmware_dao.h"
#include "../gpio/gpio_dao.h"
#include "db_keyvalue.h"

/*
 * Create the database if it doesn't exist and migrate otherways.
 */
int dao_create_db(void) {
    sqlite3 *db;

    /* Check if the file exists */
    if (access(conf->database, F_OK) != -1) {
        /* The database file exists, check if it can be opened */
        if (sqlite3_open(conf->database, &db) != SQLITE_OK) {
            /* Remove the current file because it is corrupted */
            log_message(LOG_INFO, "Removing corrupt database file\r\n");
            remove(conf->database);
        }
    }

    /* Open the existing database or create a new one */
    if (sqlite3_open(conf->database, &db) != SQLITE_OK) {
        return DB_ERR;
    }

    /* Initialize DAO modules */
    if(dao_keyvalue_init(db) == DB_ERR) {
        log_message(LOG_ERROR, "Could not successfully initialize keyvalue module database\r\n");
    }
    
    if(gpio_dao_init(db) == DB_ERR) {
        log_message(LOG_ERROR, "Could not successfully initialize GPIO module database\r\n");
    }

    if(!firmware_dao_init()) {
        log_message(LOG_ERROR, "Could not successfully initialize firmware module database\r\n");
    }

    /* Close the database */
    sqlite3_close(db);

    /* The database is successfully created */
    return DB_OK;
}

/*
 * Database closing helper
 */
inline void dao_finalize(sqlite3 *db, sqlite3_stmt* stmt) {
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

/*
 * Create a database integer result
 */
db_int* dao_create_db_int(void) {
    db_int* value = (db_int*) malloc(sizeof (db_int));
    return value;
}

/*
 * Destroy a database integer result
 */
void dao_destroy_db_int(db_int *value) {
    free(value);
}

/*
 * Create a database text result
 */
db_text* dao_create_db_text(void) {
    db_text *value = (db_text*) malloc(sizeof (db_text));
    value->value = NULL;
    return value;
}

/*
 * Destroy a database text result
 */
void dao_destroy_db_text(db_text *value) {
    /* Free the string memory if any was reserved */
    if (value->value != NULL) {
        free(value->value);
    }
    free(value);
}

/**
 * Easy SQL executer for one-line statements without external variables. The database
 * must be open and it will not be closed by this function. 
 * @param db the database to use, the database must be open already. 
 * @param sql the sql statement to execute.
 * @return DB_OK on success en DB_ERR on error. 
 */
int dao_easy_exec(sqlite3 *db, const char* sql)
{
    sqlite3_stmt *stmt;
    int rc = DB_OK;

    /* Try to prepare statement */
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not prepare SQL statement: %s\r\n", sqlite3_errmsg(db));
        rc = DB_ERR;
        goto finalize;
    }

    /* Try to execute the statement without callback */
    while ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
        switch (rc) {
            case SQLITE_DONE:
                break;
            default:
                log_message(LOG_ERROR, "Error while executing SQL statement: %s\r\n", sqlite3_errmsg(db));
                rc = DB_ERR;
                goto finalize;
                break;
        }
    }
    
    /* Success when we reach here */
    rc = DB_OK;

finalize:
    /* Finalize the statement */
    sqlite3_finalize(stmt);

    /* Return the value */
    return rc;
}
