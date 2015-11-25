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
 * File:   database.h
 * Created on September 11, 2014, 11:00 AM
 */

#ifndef DATABASE_H_
#define DATABASE_H_

#include <sqlite3.h>

/* Status parameters */
#define DB_OK		0
#define DB_ERR		1

/* Integer return structure */
typedef struct dbint {
	int status;
	char *err_msg;
	int value;
} db_int;

/* UTF-8 text return structure */
typedef struct dbtext {
	int status;
	char *err_msg;
	char* value;
} db_text;

/*
 * Create the database if it doesn't exist.
 */
int dao_create_db(void);

/*
 * Create a database integer result
 */
db_int* dao_create_db_int(void);

/*
 * Destroy a database integer result
 */
void dao_destroy_db_int(db_int *value);

/*
 * Create a database text result
 */
db_text* dao_create_db_text(void);

/*
 * Destroy a database text result
 */
void dao_destroy_db_text(db_text *value);

/*
 * Database closing helper
 */
void dao_finalize(sqlite3 *db, sqlite3_stmt* stmt);

/**
 * Easy SQL executer for one-line statements without external variables. The database
 * must be open and it will not be closed by this function. 
 * @param db the database to use, the database must be open already. 
 * @param sql the sql statement to execute.
 * @return DB_OK on success en DB_ERR on error. 
 */
int dao_easy_exec(sqlite3 *db, const char* sql);

#endif /* DATABASE_H_ */
