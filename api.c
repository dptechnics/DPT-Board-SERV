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
 * File:   api.c
 * Created on August 6, 2014, 5:28 PM
 */

#include <sys/types.h>
#include <sys/dir.h>
#include <time.h>
#include <strings.h>
#include <dirent.h>

#include <libubox/blobmsg.h>
#include <json-c/json.h>

#include "uhttpd.h"
#include "mimetypes.h"
#include "api.h"
#include "client.h"
#include "config.h"
#include "logger.h"
#include "helper.h"

/* Import modules */
#include "firmware/firmware_json_api.h"
#include "tempsensor/tempsensor_json_api.h"
#include "wifi/wifi_json_api.h"
#include "system/system_json_api.h"
#include "gpio/gpio_json_api.h"
#include "kunio/kunio_json_api.h"
#include "bluecherry/bluecherry_json_api.h"

#include "rfid/pn532/rfid_pn532_json_api.h"

/* HTTP response codes */
const struct http_response r_ok 	= { 200, "OK" };
const struct http_response r_bad_req 	= { 400, "Bad request" };
const struct http_response r_error 	= { 500, "Internal server error" };

/**
 * The get handlers table
 */
const struct f_entry get_handlers[8] = {
    { "wifi", 5, wifi_get_router },
    { "firmware", 9, firmware_get_router },
    { "tempsensor", 11, tempsensor_get_router },
    { "gpio", 5, gpio_get_router },
    { "kunio", 6, kunio_get_router },
    { "system", 7, system_get_router },
    { "bluecherry", 11, bluecherry_get_router },
    { "rfid", 5, rfid_pn532_get_router },
};

/**
 * The post handlers table
 */
const struct f_entry post_handlers[4] = {
    { "wifi", 5, wifi_post_router },
    { "firmware", 9, firmware_post_router },
    { "tempsensor", 11, tempsensor_post_router },
    { "bluecherry", 11, bluecherry_post_router },
};

/**
 * The put handlers table
 */
const struct f_entry put_handlers[2] = {
    { "gpio", 5, gpio_put_router },
    { "kunio", 6, kunio_put_router },
};

/* Lookup table for method handle lookup */
const struct f_entry* handlers[] = {
    [UH_HTTP_MSG_GET] = get_handlers,
    [UH_HTTP_MSG_POST] = post_handlers,
    [UH_HTTP_MSG_PUT] = put_handlers
};


static void write_response(struct client *cl, int code, const char *summary)
{
	/* Write response */
	write_http_header(cl, code, summary);
	ustream_printf(cl->us, "Content-Type: application/json\r\n");
	ustream_printf(cl->us, "Content-Length: %i\r\n\r\n", strlen(cl->response));

	/* Stop if this is a header only request */
	if (cl->request.method == UH_HTTP_MSG_HEAD) {
		free(cl->response);
		request_done(cl);
		return;
	}
        
        ustream_printf(cl->us, "%s", cl->response);
        free(cl->response);
        request_done(cl);
        return;
}

/**
 * Handle api requests
 * @cl the client who sent the request
 * @url the request URL
 * @pi info concerning the path
 */
void api_handle_request(struct client *cl, char *url)
{
	json_object *response = NULL;                                       /* The response */
        const struct f_entry* api_handler = NULL;                           /* The handler structure */
        json_object* (*handler)(struct client *, char *request) = NULL;     /* The handler function */

	/* Search the correct handler */
	switch(cl->request.method) {
		case UH_HTTP_MSG_GET:
			api_handler = api_get_function(url, conf->api_str_len, get_handlers, sizeof(get_handlers)/sizeof(struct f_entry));
			break;
		case UH_HTTP_MSG_POST:
			api_handler = api_get_function(url, conf->api_str_len, post_handlers, sizeof(post_handlers)/sizeof(struct f_entry));
			break;
		case UH_HTTP_MSG_PUT:
			api_handler = api_get_function(url, conf->api_str_len, put_handlers, sizeof(put_handlers)/sizeof(struct f_entry));
			break;
		default:
			api_handler = NULL;
			break;
	}
        
	/* If a handler is found execute it */
	if(api_handler){
            handler = api_handler->function;
            response = handler(cl, url + conf->api_str_len + api_handler->url_offset);
	}

	/* Write response when there is one */
	if(response){
		/* Get the string representation of the JSON object */
		const char* stringResponse = json_object_to_json_string(response);

		/* Copy the response to the response buffer */
		cl->response = (char*) malloc((strlen(stringResponse)+1)*sizeof(char));
		strcpy(cl->response, stringResponse);

		/* Free the JSON object */
		json_object_put(response);
	}else{
		/* Handle bad request */
		cl->http_status = r_bad_req;
		const char* stringResponse = "Request not supported by server.";

		/* Copy the response to the response buffer */
		cl->response = (char*) malloc((strlen(stringResponse)+1)*sizeof(char));
		strcpy(cl->response, stringResponse);
	}

	/* Write the response */
	write_response(cl, cl->http_status.code, cl->http_status.message);
}

/**
 * Get the API handler structure if any. 
 * @name the request url.
 * @offset the offset at which the method starts in the url.
 * @table the function lookup table, must be in lexical order.
 * @table_size the size of the table.
 * @return a handler struct if any, NULL otherways.
 */
const struct f_entry* api_get_function(char* url, size_t offset, const struct f_entry* table, size_t table_size)
{   
    for(int i = 0; i < table_size; ++i) {
        if(helper_str_startswith(url, table[i].name, offset)) {
                return table + i;
        }
    }

    /* Return NULL when no request could be found */
    return NULL;
}
