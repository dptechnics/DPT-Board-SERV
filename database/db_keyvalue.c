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
 * File:   db_keyvalue.c
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
#include "database.h"
#include "db_keyvalue.h"


/**
 * Initialize keyvalue database
 * @return true on success, false on error
 */
int dao_keyvalue_init(sqlite3 *db) {
    char* err_msg;
    
    char* sql = "CREATE TABLE IF NOT EXISTS keyvalue ( \
			id  INTEGER PRIMARY KEY	ASC NOT NULL, \
			key TEXT UNIQUE NOT NULL, \
			tvalue TEXT, \
			ivalue INTEGER \
			);";

    /* Try to create table */
    if (sqlite3_exec(db, sql, NULL, 0, &err_msg) != SQLITE_OK) {
        /* The table could not be constructed, exit with error */
        log_message(LOG_ERROR, "Could not create 'keyvalue' table\r\n");
        perror(err_msg);
        sqlite3_free(err_msg);
        return DB_ERR;
    }
    
    return DB_OK;
}

/*
 * Put a new integer in the configuration table
 * @key the configuration key
 * @value the value to wich to key points
 */
int dao_keyvalue_put_int(const char* key, int value) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc = DB_OK;

    /* Try to open the database */
    if (sqlite3_open(conf->database, &db) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not open database '%d': %s\r\n", conf->database, sqlite3_errmsg(db));
        rc = DB_ERR;
        goto finalize;
    }

    /* Try to prepare statement */
    if (sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO keyvalue (key, ivalue) VALUES (?, ?);", -1, &stmt, 0) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not prepare SQL statement: %s\r\n", sqlite3_errmsg(db));
        rc = DB_ERR;
        goto finalize;
    }

    /* Try to bind char parameter */
    if (sqlite3_bind_text(stmt, 1, key, -1, 0) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not bind text '%s': %s\r\n", key, sqlite3_errmsg(db));
        rc = DB_ERR;
        goto finalize;
    }

    /* Try to bind integer parameter */
    if (sqlite3_bind_int(stmt, 2, value) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not bind integer '%d': %s\r\n", value, sqlite3_errmsg(db));
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
    dao_finalize(db, stmt);

    /* Return the value */
    return rc;
}

/*
 * Put a new text value in the configuration table
 * @key the configuration key
 * @value the value to which to key points
 */
int dao_keyvalue_put_text(const char* key, const char* value) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc = DB_OK;

    /* Try to open the database */
    if (sqlite3_open(conf->database, &db) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not open database '%d': %s\r\n", conf->database, sqlite3_errmsg(db));
        rc = DB_ERR;
        goto finalize;
    }

    /* Try to prepare statement */
    if (sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO keyvalue (key, tvalue) VALUES (?, ?);", -1, &stmt, 0) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not prepare SQL statement: %s\r\n", sqlite3_errmsg(db));
        rc = DB_ERR;
        goto finalize;
    }

    /* Try to bind char parameter */
    if (sqlite3_bind_text(stmt, 1, key, -1, 0) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not bind text '%s': %s\r\n", key, sqlite3_errmsg(db));
        rc = DB_ERR;
        goto finalize;
    }

    /* Try to bind integer parameter */
    if (sqlite3_bind_text(stmt, 2, value, -1, 0) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not bind text '%s': %s\r\n", value, sqlite3_errmsg(db));
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
    dao_finalize(db, stmt);

    /* Return the value */
    return rc;
}

/*
 * Update configuration integer value
 * @key the configuration key
 * @value the value to which to key points
 */
int dao_keyvalue_edit_int(const char* key, int value) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc = DB_OK;

    /* Try to open the database */
    if (sqlite3_open(conf->database, &db) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not open database '%d': %s\r\n", conf->database, sqlite3_errmsg(db));
        rc = DB_ERR;
        goto finalize;
    }

    /* Try to prepare statement */
    if (sqlite3_prepare_v2(db, "UPDATE keyvalue SET ivalue = ? WHERE key = ?;", -1, &stmt, 0) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not prepare SQL statement: %s\r\n", sqlite3_errmsg(db));
        rc = DB_ERR;
        goto finalize;
    }

    /* Try to bind key parameter */
    if (sqlite3_bind_text(stmt, 2, key, -1, 0) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not bind text '%s': %s\r\n", key, sqlite3_errmsg(db));
        rc = DB_ERR;
        goto finalize;
    }

    /* Try to bind integer parameter */
    if (sqlite3_bind_int(stmt, 1, value) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not bind integer '%d': %s\r\n", value, sqlite3_errmsg(db));
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
    dao_finalize(db, stmt);

    /* Return the value */
    return rc;
}

/*
 * Update configuration text value
 * @key the configuration key
 * @value the value to which to key points
 */
int dao_keyvalue_edit_text(const char* key, const char* value) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc = DB_OK;

    /* Try to open the database */
    if (sqlite3_open(conf->database, &db) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not open database '%d': %s\r\n", conf->database, sqlite3_errmsg(db));
        rc = DB_ERR;
        goto finalize;
    }

    /* Try to prepare statement */
    if (sqlite3_prepare_v2(db, "UPDATE keyvalue SET tvalue = ? WHERE key = ?;", -1, &stmt, 0) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not prepare SQL statement: %s\r\n", sqlite3_errmsg(db));
        rc = DB_ERR;
        goto finalize;
    }

    /* Try to bind key parameter */
    if (sqlite3_bind_text(stmt, 2, key, -1, 0) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not bind text '%s': %s\r\n", key, sqlite3_errmsg(db));
        rc = DB_ERR;
        goto finalize;
    }

    /* Try to bind integer parameter */
    if (sqlite3_bind_text(stmt, 1, value, -1, 0) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not bind text '%s': %s\r\n", value, sqlite3_errmsg(db));
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
    dao_finalize(db, stmt);

    /* Return the value */
    return rc;
}

/*
 * Get an integer configuration value
 * @key the configuration key
 * @return the return value, with status, this should be freed after use
 */
db_int* dao_keyvalue_get_int(const char* key) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    db_int *retvalue = dao_create_db_int();
    int rc;

    /* Try to open the database */
    if (sqlite3_open(conf->database, &db) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not open database '%d': %s\r\n", conf->database, sqlite3_errmsg(db));
        retvalue->status = DB_ERR;
        retvalue->err_msg = "could not open database";
        goto finalize;
    }

    /* Try to prepare statement */
    if (sqlite3_prepare_v2(db, "SELECT ivalue FROM keyvalue WHERE key = ?", -1, &stmt, 0) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not prepare SQL statement: %s\r\n", sqlite3_errmsg(db));
        retvalue->status = DB_ERR;
        retvalue->err_msg = "could not prepare statement";
        goto finalize;
    }

    /* Try to bind char parameter */
    if (sqlite3_bind_text(stmt, 1, key, -1, 0) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not bind text '%s': %s\r\n", key, sqlite3_errmsg(db));
        retvalue->status = DB_ERR;
        retvalue->err_msg = "could not bind key parameter";
        goto finalize;
    }

    /* Try to execute the statement without callback */
    while ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
        switch (rc) {
            case SQLITE_BUSY:
                log_message(LOG_ERROR, "The SQL database is locked: %s\r\n", sqlite3_errmsg(db));
                retvalue->status = DB_ERR;
                retvalue->err_msg = "database is locked";
                goto finalize;
                break;
            case SQLITE_ERROR:
                log_message(LOG_ERROR, "Could not fetch data from table: %s\r\n", sqlite3_errmsg(db));
                retvalue->status = DB_ERR;
                retvalue->err_msg = "could not fetch data from table";
                goto finalize;
                break;
            case SQLITE_ROW:
                /* This statement contains only one column */
                retvalue->status = DB_OK;
                retvalue->value = sqlite3_column_int(stmt, 0);
                break;
        }
    }
    
    /* Success when we reach here */
    rc = DB_OK;

finalize:
    /* Finalize the statement */
    dao_finalize(db, stmt);

    /* Return the value */
    return retvalue;
}

/*
 * Get a text configuration value
 * @key the configuration key
 * @return the return value, with status, this should be freed after use
 */
db_text* dao_keyvalue_get_text(const char* key) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    db_text *retvalue = dao_create_db_text();
    int rc;

    /* Try to open the database */
    if (sqlite3_open(conf->database, &db) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not open database '%d': %s\r\n", conf->database, sqlite3_errmsg(db));
        retvalue->status = DB_ERR;
        retvalue->err_msg = "could not open database";
        goto finalize;
    }

    /* Try to prepare statement */
    if (sqlite3_prepare_v2(db, "SELECT tvalue FROM keyvalue WHERE key = ?", -1, &stmt, 0) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not prepare SQL statement: %s\r\n", sqlite3_errmsg(db));
        retvalue->status = DB_ERR;
        retvalue->err_msg = "could not prepare statement";
        goto finalize;
    }

    /* Try to bind char parameter */
    if (sqlite3_bind_text(stmt, 1, key, -1, NULL) != SQLITE_OK) {
        /* Return with error */
        log_message(LOG_ERROR, "Could not bind text '%s': %s\r\n", key, sqlite3_errmsg(db));
        retvalue->status = DB_ERR;
        retvalue->err_msg = "could not bind key parameter";
        goto finalize;
    }

    /* Try to execute the statement without callback */
    while ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
        switch (rc) {
            case SQLITE_BUSY:
                log_message(LOG_ERROR, "The SQL database is locked: %s\r\n", sqlite3_errmsg(db));
                retvalue->status = DB_ERR;
                retvalue->err_msg = "database is locked";
                goto finalize;
                break;
            case SQLITE_ERROR:
                log_message(LOG_ERROR, "Could not fetch data from table: %s\r\n", sqlite3_errmsg(db));
                retvalue->status = DB_ERR;
                retvalue->err_msg = "could not fetch data from table";
                goto finalize;
                break;
            case SQLITE_ROW:
                /* This statement contains only one column */
                retvalue->status = DB_OK;

                /* Copy value into memory */
                rc = sqlite3_column_bytes(stmt, 0);
                retvalue->value = (char*) malloc((rc + 1) * sizeof (char));
                memcpy(retvalue->value, sqlite3_column_text(stmt, 0), rc + 1);
                break;
        }
    }
    
    /* Success when we reach here */
    rc = DB_OK;

finalize:
    /* Finalize the statement */
    dao_finalize(db, stmt);

    /* Return the value */
    return retvalue;
}
