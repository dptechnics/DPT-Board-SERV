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
 * File:   client.c
 * Created on May 10, 2014, 5:28 PM
 */

#include <libubox/blobmsg.h>
#include <ctype.h>

#include "config.h"
#include "listen.h"
#include "uhttpd.h"
#include "client.h"

/* The list of connected clients */
static LIST_HEAD(clients);

/* The number of connected clients */
int n_clients = 0;

/* Status flag for currently selected client */
static bool client_done = false;

/* Array giving string representation to 'http_version' enum */
const char * const http_versions[] = {
	[UH_HTTP_VER_0_9] = "HTTP/0.9",
	[UH_HTTP_VER_1_0] = "HTTP/1.0",
	[UH_HTTP_VER_1_1] = "HTTP/1.1",
};

/* Array giving string representations to 'http_method' enum */
const char * const http_methods[] = {
	[UH_HTTP_MSG_GET] = "GET",
	[UH_HTTP_MSG_POST] = "POST",
	[UH_HTTP_MSG_HEAD] = "HEAD",
	[UH_HTTP_MSG_PUT] = "PUT",
};

/**
 * Write a http header to a client
 * @client the client to write the header to
 * @code the http status code t o write
 * @summary the http status code info, for example if code = 200, summary = "Ok"
 */
void write_http_header(struct client *cl, int code, const char *summary)
{
	struct http_request *r = &cl->request;
	const char *enc = "Transfer-Encoding: chunked\r\n";
	const char *conn;

	/* If no chunked transfer is used, remove the encoding line */
	if (!uh_use_chunked(cl))
		enc = "";

	/* Check if connection should be closed or kept open after request */
	if (r->connection_close)
		conn = "Connection: close";
	else
		conn = "Connection: Keep-Alive";

	/* Send the header to the client */
	ustream_printf(cl->us, "%s %03i %s\r\n%s\r\n%s",
		http_versions[cl->request.version],
		code, summary, conn, enc);

	/* If this is a Keep-Alive connection, send the keep alive time */
	if (!r->connection_close)
		ustream_printf(cl->us, "Keep-Alive: timeout=%d\r\n", conf->keep_alive_time);
}

/**
 * Close this client connection
 * @client the client to close the connection from
 */
static void close_connection(struct client *cl)
{
	cl->state = CLIENT_STATE_CLOSE;
	cl->us->eof = true;
	ustream_state_change(cl->us);
}

/**
 * Free the dispatch method resources
 * @client the client to free the resources from
 */
static void dispatch_done(struct client *cl)
{
	if (cl->dispatch.free)
		cl->dispatch.free(cl);
	if (cl->dispatch.req_free)
		cl->dispatch.req_free(cl);
}

/**
 * This function is called when a client times out. The connection
 * should then be closed.
 * @timeout: the timeout event from the uloop event loop
 */
static void timeout_event_handler(struct uloop_timeout *timeout)
{
	/* Get the client that caused the timeout event */
	struct client *cl = container_of(timeout, struct client, timeout);

	/* Close the connection */
	close_connection(cl);
}

/**
 * This handler should be installed on the event loop timeout event
 * when a keepalive connections is used. It wil keep the connection
 * open when needed and close when real timeout has happened.
 * @timeout: the timeout event from the uloop event loop
 */
static void polling_event_handler(struct uloop_timeout *timeout)
{
	/* Get the client that caused the timeout event */
	struct client *cl = container_of(timeout, struct client, timeout);

	/* Set timeout when the client had made request, network timeout otherwise */
	int msec = cl->requests > 0 ? conf->keep_alive_time : conf->network_timeout;

	/* Install closing event handler on the connection */
	cl->timeout.cb = timeout_event_handler;
	uloop_timeout_set(&cl->timeout, msec * 1000);
	cl->us->notify_read(cl->us, 0);
}

/**
 * Poll the connection to keep it alive
 */
static void poll_connection(struct client *cl)
{
	/* Install polling event handler on the connection */
	cl->timeout.cb = polling_event_handler;
	uloop_timeout_set(&cl->timeout, 1);
}

/**
 * Signal a request is done and set the connection to wait
 * for another request from the client.
 * @cl the client from which the request is done
 */
void request_done(struct client *cl)
{
	/* Send EOF to client and free dispatch resources */
	uh_chunk_eof(cl);
	dispatch_done(cl);

	/* Set the dispatch pointers to zero */
	memset(&cl->dispatch, 0, sizeof(cl->dispatch));

	/* If this is no Keep-Alive connection close it */
	if (!conf->keep_alive_time || cl->request.connection_close){
		close_connection(cl);
	} else {
		/* Else wait for new requests and poll the connection to keep it alive */
		cl->state = CLIENT_STATE_INIT;
		cl->requests++;
		poll_connection(cl);
	}
}

/**
 * Send an error message to the browser
 * @cl the client to send the error message to
 * @code the error code to write to the client
 * @summary the code description, for example code = 500, summary = "Internal Server Error"
 * @fmt optional error information
 */
void __printf(4, 5) send_client_error(struct client *cl, int code, const char *summary, const char *fmt, ...)
{
	va_list arg;

	/* Write the header with the error code */
	write_http_header(cl, code, summary);

	/* Set the content type to html */
	ustream_printf(cl->us, "Content-Type: text/html\r\n\r\n");

	/* Send the code summary in heading */
	uh_chunk_printf(cl, "<h1>%s</h1>", summary);

	/* If there is optional info send it */
	if (fmt) {
		va_start(arg, fmt);
		uh_chunk_vprintf(cl, fmt, arg);
		va_end(arg);
	}

	/* End the request */
	request_done(cl);
}

/**
 * Send an error message to the browser
 * @cl the client to send the error message to
 * @code the error code to write to the client
 * @summary the code description, for example code = 500, summary = "Internal Server Error"
 * @fmt optional error information
 */
void __printf(4,5) client_send_error(struct client *cl, int code, const char *summary, const char *format, ...){
    va_list arg;
    char buf[256];
    int len;
    
    /* Write header with error code */
    write_http_header(cl, code, summary);
    ustream_printf(cl->us, "Content-Type: text/html\r\n");
    
    /* Prepare buffer */
    len = snprintf(buf, 256, "<h1>%s</h1>", summary);
    if(format) {
        va_start(arg, format);
        len += snprintf(buf + len, 256 - len, format, arg);
        va_end(arg);
    }

    ustream_printf(cl->us, "Content-Length: %d\r\n\r\n", len);
    ustream_printf(cl->us, "%s", buf);
    
    /* End the request */
    request_done(cl);
}


/**
 * Handle header errors.
 * @cl the client that send wrong headers
 * @code the error code
 * @summary the code description
 */
static void header_error(struct client *cl, int code, const char *summary)
{
	send_client_error(cl, code, summary, NULL);
	close_connection(cl);
}

/**
 * This helper method is used to find the index of the http_methods
 * and http_version enums from the header string.
 * @list the list of possible strings
 * @max the length of the list to search in
 * @str the string to search in
 */
static int find_idx(const char * const *list, int max, const char *str)
{
	int i;

	for (i = 0; i < max; i++)
		if (!strcmp(list[i], str))
			return i;

	return -1;
}

/**
 * Parse an incoming client request. This is installed as a handler
 * @cl: the client that sent the request
 * @data: the request data
 */
static int parse_client_request(struct client *cl, char *data)
{
	struct http_request *req = &cl->request;
	char *type, *path, *version;
	int h_method, h_version;

	/* Split the the data on spaces */
	type = strtok(data, " ");
	path = strtok(NULL, " ");
	version = strtok(NULL, " ");
	if (!type || !path || !version)
		return CLIENT_STATE_DONE;

	/* Add the path to the client header */
	blobmsg_add_string(&cl->hdr, "URL", path);

	/* Clear the previous request info from the client */
	memset(&cl->request, 0, sizeof(cl->request));

	/* TODO: bad request bug on PUT requests */
	/* Find the enums corresponding to the method and type */
	h_method = find_idx(http_methods, ARRAY_SIZE(http_methods), type);
	h_version = find_idx(http_versions, ARRAY_SIZE(http_versions), version);

	/* Check if the method is supported */
	if (h_method < 0 || h_version < 0) {
		req->version = UH_HTTP_VER_1_0;
		return CLIENT_STATE_DONE;
	}

	/* Save the http method en version */
	req->method = h_method;
	req->version = h_version;

	/* Close connection when needed */
	if (req->version < UH_HTTP_VER_1_1 || req->method == UH_HTTP_MSG_POST)
		req->connection_close = true;

	/* Set the state as header parsed */
	return CLIENT_STATE_HEADER;
}

/**
 * This function is called when a client makes a new request.
 * @cl the client that made the request
 * @buf the buffer containing the request data
 * @len the lenght of the request
 */
static bool client_init_handler(struct client *cl, char *buf, int len)
{
	char *newline;

	/* Get the first newline in the the header, if there is no newlien
	 * the header is faulty */
	newline = strstr(buf, "\r\n");
	if (!newline)
		return false;

	/* If the buffer only contains a newline, consume them */
	if (newline == buf) {
		ustream_consume(cl->us, 2);
		return true;
	}

	/* Nullterminate the string */
	*newline = 0;

	/* Parse the header in the buffer and consume the stream */
	blob_buf_init(&cl->hdr, 0);
	cl->state = parse_client_request(cl, buf);
	ustream_consume(cl->us, newline + 2 - buf);

	/* Return an error when the header is malformed */
	if (cl->state == CLIENT_STATE_DONE)
		header_error(cl, 400, "Bad Request");

	return true;
}

/**
 * This function should be called when header
 * parsing is complete.
 * @cl: the client to parse the header from
 */
static void client_header_complete(struct client *cl)
{
	struct http_request *r = &cl->request;

	/* If a contiuation is expected return status 100 */
	if (r->expect_cont)
		ustream_printf(cl->us, "HTTP/1.1 100 Continue\r\n\r\n");

	/* Older browser compatibility */
	switch(r->ua) {
	case UH_UA_MSIE_OLD:
		if (r->method != UH_HTTP_MSG_POST){
			break;
		} else {
			r->connection_close = true;
		}
		break;
	case UH_UA_SAFARI:
		r->connection_close = true;
		break;
	default:
		break;
	}

	uh_handle_request(cl);
}

/**
 * Parse a client header line not containing a newline on the end.
 * @cl the client who sent the header
 * @data a line from the client header
 */
static void client_parse_header(struct client *cl, char *data)
{
	struct http_request *r = &cl->request;
	char *err;
	char *name;
	char *val;

	/* Parse post data if there is any */
	if (!*data) {
		uloop_timeout_cancel(&cl->timeout);
		cl->state = CLIENT_STATE_DATA;
		client_header_complete(cl);
		return;
	}

	/* Split the header into name-value pair, when there is no more
	 * data stop parsing headers */
	val = uh_split_header(data);
	if (!val) {
		cl->state = CLIENT_STATE_DONE;
		return;
	}

	/* Set all characters to lower case */
	for (name = data; *name; name++)
		if (isupper(*name))
			*name = tolower(*name);

	/* Parse function for every possible header name-value pair */
	if (!strcmp(data, "expect")) {
		if (!strcasecmp(val, "100-continue"))
			r->expect_cont = true;
		else {
			header_error(cl, 412, "Precondition Failed");
			return;
		}
	} else if (!strcmp(data, "content-length")) {
		r->content_length = strtoul(val, &err, 0);
		if (err && *err) {
			header_error(cl, 400, "Bad Request");
			return;
		}
	} else if (!strcmp(data, "transfer-encoding")) {
		if (!strcmp(val, "chunked"))
			r->transfer_chunked = true;
	} else if (!strcmp(data, "connection")) {
		if (!strcasecmp(val, "close"))
			r->connection_close = true;
	} else if (!strcmp(data, "user-agent")) {
		char *str;

		if (strstr(val, "Opera"))
			r->ua = UH_UA_OPERA;
		else if ((str = strstr(val, "MSIE ")) != NULL) {
			r->ua = UH_UA_MSIE_NEW;
			if (str[5] && str[6] == '.') {
				switch (str[5]) {
				case '6':
					if (strstr(str, "SV1")) {
						break;
					}
					r->ua = UH_UA_MSIE_OLD;
					break;
				case '5':
				case '4':
					r->ua = UH_UA_MSIE_OLD;
					break;
				}
			}
		}
		else if (strstr(val, "Chrome/"))
			r->ua = UH_UA_CHROME;
		else if (strstr(val, "Safari/") && strstr(val, "Mac OS X"))
			r->ua = UH_UA_SAFARI;
		else if (strstr(val, "Gecko/"))
			r->ua = UH_UA_GECKO;
		else if (strstr(val, "Konqueror"))
			r->ua = UH_UA_KONQUEROR;
	}

	/* Add the key value pair to hdr header data */
	blobmsg_add_string(&cl->hdr, data, val);

	/* Flag to state we are ready to parse the next header line */
	cl->state = CLIENT_STATE_HEADER;
}

/**
 * Retreive client post data.
 * @cl the client who sent de data.
 */
void client_post_data(struct client *cl)
{
	struct dispatch *d = &cl->dispatch;
	struct http_request *r = &cl->request;
	char *buf;
	int len;

	/* If there is no data to handle return */
	if (cl->state == CLIENT_STATE_DONE) {
		return;
	}

	while (1) {
		char *sep;
		int offset = 0;
		int cur_len;

		/* Get the data buffer */
		buf = ustream_get_read_buf(cl->us, &len);
		if (!buf || !len)
			break;

		/* If there is data to be sent return */
		if (!d->data_send){
			return;
		}

		/* Get the current lenght of the buffer */
		cur_len = min(r->content_length, len);
		if (cur_len) {
			/* Stop if the data is blocked */
			if (d->data_blocked)
				break;

			if (d->data_send) {
				cur_len = d->data_send(cl, buf, cur_len);
			}

			r->content_length -= cur_len;
			ustream_consume(cl->us, cur_len);
			continue;
		}

		/* Stop is the transfer is not chunked */
		if (!r->transfer_chunked)
			break;

		if (r->transfer_chunked > 1)
			offset = 2;

		/* Get the POST data separator */
		sep = strstr(buf + offset, "\r\n");
		if (!sep)
			break;

		/* Nullterminate the string */
		*sep = 0;
		printf("Separator: %s\r\n", sep);

		r->content_length = strtoul(buf + offset, &sep, 16);
		r->transfer_chunked++;
		ustream_consume(cl->us, sep + 2 - buf);

		/* invalid chunk length */
		if (sep && *sep) {
			r->content_length = 0;
			r->transfer_chunked = 0;
			break;
		}

		/* empty chunk == eof */
		if (!r->content_length) {
			r->transfer_chunked = false;
			break;
		}
	}

	/* Read the parameter into the buffer */
	buf = ustream_get_read_buf(cl->us, &len);
	if (!r->content_length && !r->transfer_chunked && cl->state != CLIENT_STATE_DONE) {
		if (cl->dispatch.data_done)
			cl->dispatch.data_done(cl);

		cl->state = CLIENT_STATE_DONE;
	}
}

/**
 * Handler called for POST data
 * @cl the client who sent the data
 * @buf the buffer containing the data
 * @len the length of the data
 */
static bool client_data_handler(struct client *cl, char *buf, int len)
{
	client_post_data(cl);
	return false;
}

/**
 * Handler called when the header should be parsed.
 * @cl the client who sent the header.
 * @buf the buffer containing the header.
 * @len the length of the buffer.
 */
static bool client_header_handler(struct client *cl, char *buf, int len)
{
	char *newline;
	int line_len;

	/* Get the first line of the data until a newline */
	newline = strstr(buf, "\r\n");
	if (!newline){
		return false;
	}

	/* Nullterminate the string buffer on newline when
	 * newline is not followed by another newline. Otherwise
	 * buffer contains post data
	 */
	if(!strstr(newline + 2, "\r\n")){
		// Save the found post data
		cl->ispostdata = true;
		cl->postdata = (char*) realloc(cl->postdata, (strlen(newline+2)+1)*sizeof(char));
		strcpy(cl->postdata, newline + 2);
	}

	*newline = 0;

	/* Parse the header */
	client_parse_header(cl, buf);

	/* Consume the line in the stream */
	line_len = newline + 2 - buf;
	ustream_consume(cl->us, line_len);

	/* Parse client data if there is any */
	if (cl->state == CLIENT_STATE_DATA){
		return client_data_handler(cl, newline + 2, len - line_len);
	}

	return true;
}

typedef bool (*read_cb_t)(struct client *cl, char *buf, int len);
static read_cb_t read_cbs[] = {
	[CLIENT_STATE_INIT] 	= client_init_handler,
	[CLIENT_STATE_HEADER] 	= client_header_handler,
	[CLIENT_STATE_DATA] 	= client_data_handler,
};

/**
 * Read data from client. Read the request and parse
 * all headers and data.
 * @cl the client to read from.
 */
void read_from_client(struct client *cl)
{
	struct ustream *us = cl->us;
	char *str;
	int len;

	client_done = false;
	do {
		/* Read sata if there is any */
		str = ustream_get_read_buf(us, &len);
		if (!str || !len)
			break;

		if (cl->state >= array_size(read_cbs) || !read_cbs[cl->state])
			break;

		/* Call different handlers and parse */
		if (!read_cbs[cl->state](cl, str, len)) {
			if (len == us->r.buffer_len &&
			    cl->state != CLIENT_STATE_DATA)
				header_error(cl, 413, "Request Entity Too Large");
			break;
		}
	} while (!client_done);
}

/**
 * Close the connection to the client.
 * @cl the client to close the connection from.
 */
static void client_close(struct client *cl)
{
	/* Cleanup when necessary */
	if (cl->refcount) {
		cl->state = CLIENT_STATE_CLEANUP;
		return;
	}

	/* Free all resources */
	if(cl->ispostdata)
		free(cl->postdata);
	client_done = true;
	n_clients--;
	dispatch_done(cl);
	uloop_timeout_cancel(&cl->timeout);
	ustream_free(&cl->sfd.stream);
	close(cl->sfd.fd.fd);
	list_del(&cl->list);
	blob_buf_free(&cl->hdr);
	free(cl);



	/* Unblock other listeners so pending clients can be handled */
	unblock_listeners();
}

/**
 * Close client if possible.
 * @cl the client the close.
 */
void client_notify_state(struct client *cl)
{
	struct ustream *s = cl->us;

	/* Do not close in cleanup or DATA state */
	if (!s->write_error && cl->state != CLIENT_STATE_CLEANUP) {
		if (cl->state == CLIENT_STATE_DATA)
			return;

		/* Dont close when the stream is not fully read */
		if (!s->eof || s->w.data_bytes)
			return;
	}

	return client_close(cl);
}

/**
 * Read from a ustream.
 * @s the stream to read from.
 * @bytes the number of bytes to read.
 */
static void client_ustream_read_handler(struct ustream *s, int bytes)
{
	struct client *cl = container_of(s, struct client, sfd.stream);

	read_from_client(cl);
}

/**
 * Call the write dispatcher installed in the client owning the stream
 * and writes the bytes.
 * @s the stream to write to.
 * @bytes not used.
 */
static void client_ustream_write_handler(struct ustream *s, int bytes)
{
	struct client *cl = container_of(s, struct client, sfd.stream);

	if (cl->dispatch.write_cb)
		cl->dispatch.write_cb(cl);
}

/**
 * Handler called to notify state.
 * @s the stream owned by the client to notify state on.
 */
static void client_notify_state_handler(struct ustream *s)
{
	struct client *cl = container_of(s, struct client, sfd.stream);

	client_notify_state(cl);
}

/**
 * Set the address to reply to
 * @addr the address to set
 * @src the source address
 */
static void set_addr(struct uh_addr *addr, void *src)
{
	struct sockaddr_in *sin = src;
	struct sockaddr_in6 *sin6 = src;

	addr->family = sin->sin_family;
	if (addr->family == AF_INET) {
		addr->port = ntohs(sin->sin_port);
		memcpy(&addr->in, &sin->sin_addr, sizeof(addr->in));
	} else {
		addr->port = ntohs(sin6->sin6_port);
		memcpy(&addr->in6, &sin6->sin6_addr, sizeof(addr->in6));
	}
}

/**
 * Accept a new client
 * @fd the socket to accept the client on
 * @tls true if this client has https
 */
bool accept_client(int fd, bool tls)
{
	static struct client *next_client;
	struct client *cl;
	unsigned int sl;
	int sfd;
	static int client_id = 0;
	struct sockaddr_in6 addr;

	/* If the list has no space enlarge it with one */
	if (!next_client)
		next_client = calloc(1, sizeof(*next_client));

	cl = next_client;

	/* Set all the correct addresses */
	sl = sizeof(addr);
	sfd = accept(fd, (struct sockaddr *) &addr, &sl);
	if (sfd < 0)
		return false;

	set_addr(&cl->peer_addr, &addr);
	sl = sizeof(addr);
	getsockname(sfd, (struct sockaddr *) &addr, &sl);
	set_addr(&cl->srv_addr, &addr);

	/* Attach all handlers */
	cl->us = &cl->sfd.stream;
	cl->us->notify_read = client_ustream_read_handler;
	cl->us->notify_write = client_ustream_write_handler;
	cl->us->notify_state = client_notify_state_handler;

	/* Initialise stream for string data */
	cl->us->string_data = true;
	ustream_fd_init(&cl->sfd, sfd);

	/* Add the client to the list and poll connection */
	poll_connection(cl);
	list_add_tail(&cl->list, &clients);

	/* Do some administration */
	next_client = NULL;
	n_clients++;
	cl->id = client_id++;

	return true;
}

/**
 * Close all clients
 */
void close_sockets(void)
{
	struct client *cl;

	uloop_done();
	close_listeners();
	list_for_each_entry(cl, &clients, list) {
		close(cl->sfd.fd.fd);
		if (cl->dispatch.close_fds)
			cl->dispatch.close_fds(cl);
	}
}
