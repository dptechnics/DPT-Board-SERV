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
 * File:   client.h
 * Created on May 10, 2014, 5:28 PM
 */

#ifndef CLIENT_H_
#define CLIENT_H_

/**
 * Write a http header to a client
 * @cl the client to write the header to
 * @code the http status code t o write
 * @summary the http status code info, for example if code = 200, summary = "Ok"
 */
void write_http_header(struct client *cl, int code, const char *summary);

/**
 * Signal a request is done and set the connection to wait
 * for another request from the client.
 * @cl the client from which the request is done
 */
void request_done(struct client *cl);

/**
 * Send an error message to the browser
 * @cl the client to send the error message to
 * @code the error code to write to the client
 * @summary the code description, for example code = 500, summary = "Internal Server Error"
 * @fmt optional error information
 */
void __printf(4, 5) send_client_error(struct client *cl, int code, const char *summary, const char *fmt, ...);

/**
 * Send an error message to the browser
 * @cl the client to send the error message to
 * @code the error code to write to the client
 * @summary the code description, for example code = 500, summary = "Internal Server Error"
 * @fmt optional error information
 */
void __printf(4,5) client_send_error(struct client *cl, int code, const char *summary, const char *format, ...);

/**
 * Parse client POST data.
 * @cl the client who sent de data
 */
void client_post_data(struct client *cl);

/**
 * Read data from client. Read the request and parse
 * all headers and data.
 * @cl the client to read from
 */
void read_from_client(struct client *cl);

/**
 * Close client if possible
 * @cl the client the close
 */
void client_notify_state(struct client *cl);

/**
 * Accept a new client
 * @fd the socket to accept the client on
 * @tls true if this client has https
 */
bool accept_client(int fd, bool tls);

/**
 * Close all clients
 */
void close_sockets(void);

#endif /* CLIENT_H_ */
