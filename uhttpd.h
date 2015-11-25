/*
 * uhttpd - Tiny single-threaded httpd
 *
 *   Copyright (C) 2010-2013 Jo-Philipp Wich <xm@subsignal.org>
 *   Copyright (C) 2013 Felix Fietkau <nbd@openwrt.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __UHTTPD_H
#define __UHTTPD_H

#include <netinet/in.h>
#include <limits.h>
#include <dirent.h>

#include <libubox/list.h>
#include <libubox/uloop.h>
#include <libubox/ustream.h>
#include <libubox/blob.h>
#include <libubox/utils.h>

#ifdef HAVE_TLS
#include <libubox/ustream-ssl.h>
#endif

#include "utils.h"
#include "config.h"

#define UH_LIMIT_CLIENTS	64

#define __enum_header(_name, _val) HDR_##_name,
#define __blobmsg_header(_name, _val) [HDR_##_name] = { .name = #_val, .type = BLOBMSG_TYPE_STRING },

struct client;

struct auth_realm {
    struct list_head list;
    const char *path;
    const char *user;
    const char *pass;
};

enum http_method {
    UH_HTTP_MSG_GET,
    UH_HTTP_MSG_POST,
    UH_HTTP_MSG_HEAD,
    UH_HTTP_MSG_PUT,
};

enum http_version {
    UH_HTTP_VER_0_9,
    UH_HTTP_VER_1_0,
    UH_HTTP_VER_1_1,
};

enum http_user_agent {
    UH_UA_UNKNOWN,
    UH_UA_GECKO,
    UH_UA_CHROME,
    UH_UA_SAFARI,
    UH_UA_MSIE,
    UH_UA_KONQUEROR,
    UH_UA_OPERA,
    UH_UA_MSIE_OLD,
    UH_UA_MSIE_NEW,
};

struct http_request {
    enum http_method method;
    enum http_version version;
    enum http_user_agent ua;
    int redirect_status;
    int content_length;
    bool expect_cont;
    bool connection_close;
    uint8_t transfer_chunked;
    const struct auth_realm *realm;
};

enum client_state {
    CLIENT_STATE_INIT,
    CLIENT_STATE_HEADER,
    CLIENT_STATE_DATA,
    CLIENT_STATE_DONE,
    CLIENT_STATE_CLOSE,
    CLIENT_STATE_CLEANUP,
};

struct interpreter {
    struct list_head list;
    const char *path;
    const char *ext;
};

struct path_info {
    const char *root;
    const char *phys;
    const char *name;
    const char *info;
    const char *query;
    const char *auth;
    bool redirected;
    struct stat stat;
    const struct interpreter *ip;
};

struct env_var {
    const char *name;
    const char *value;
};

struct relay {
    struct ustream_fd sfd;
    struct uloop_process proc;
    struct uloop_timeout timeout;
    struct client *cl;

    bool process_done;
    bool error;
    bool skip_data;

    int ret;
    int header_ofs;

    void (*header_cb)(struct relay *r, const char *name, const char *value);
    void (*header_end)(struct relay *r);
    void (*close)(struct relay *r, int ret);
};

struct dispatch_proc {
    struct uloop_timeout timeout;
    struct blob_buf hdr;
    struct uloop_fd wrfd;
    struct relay r;
    int status_code;
    char *status_msg;
};

struct dispatch_handler {
    struct list_head list;
    bool script;

    bool (*check_url)(const char *url);
    bool (*check_path)(struct path_info *pi, const char *url);
    void (*handle_request)(struct client *cl, char *url, struct path_info *pi);
};

struct dispatch {
    int (*data_send)(struct client *cl, const char *data, int len);
    void (*data_done)(struct client *cl);
    void (*write_cb)(struct client *cl);
    void (*close_fds)(struct client *cl);
    void (*free)(struct client *cl);

    void *req_data;
    void (*req_free)(struct client *cl);

    bool data_blocked;

    union {

        struct {
            struct blob_attr **hdr;
            int fd;
        } file;
        struct dispatch_proc proc;
#ifdef HAVE_UBUS
        struct dispatch_ubus ubus;
#endif
    };
};

/* HTTP response */
struct http_response {
    int code;
    const char* message;
};

extern const struct http_response r_ok;
extern const struct http_response r_bad_req;
extern const struct http_response r_error;

struct client {
    struct list_head list;
    int refcount;
    int id;
    bool use_chunked;

    struct ustream *us;
    struct ustream_fd sfd;
    struct uloop_timeout timeout;
    int requests;

    enum client_state state;
    bool tls;

    struct http_request request;
    struct uh_addr srv_addr, peer_addr;

    struct blob_buf hdr;
    struct dispatch dispatch;
    char *response;
    struct http_response http_status;
    int readidx;
    bool ispostdata;
    char *postdata;
};

extern char uh_buf[WORKING_BUFF_SIZE];
extern int n_clients;
extern const char * const http_versions[];
extern const char * const http_methods[];
extern struct dispatch_handler cgi_dispatch;


bool uh_use_chunked(struct client *cl);
void uh_chunk_write(struct client *cl, const void *data, int len);
void uh_chunk_vprintf(struct client *cl, const char *format, va_list arg);

void __printf(2, 3)
uh_chunk_printf(struct client *cl, const char *format, ...);

void uh_chunk_eof(struct client *cl);
void uh_handle_request(struct client *cl);

void uh_auth_add(const char *path, const char *user, const char *pass);
bool uh_auth_check(struct client *cl, struct path_info *pi);

void uh_interpreter_add(const char *ext, const char *path);
void uh_dispatch_add(struct dispatch_handler *d);

void uh_relay_open(struct client *cl, struct relay *r, int fd, int pid);
void uh_relay_close(struct relay *r, int ret);
void uh_relay_free(struct relay *r);
void uh_relay_kill(struct client *cl, struct relay *r);


bool uh_create_process(struct client *cl, struct path_info *pi, char *url,
        void (*cb)(struct client *cl, struct path_info *pi, char *url));

int uh_plugin_init(const char *name);
void uh_plugin_post_init(void);

static inline void uh_client_ref(struct client *cl) {
    cl->refcount++;
}

static inline void uh_client_unref(struct client *cl) {
    if (--cl->refcount)
        return;

    if (cl->state == CLIENT_STATE_CLEANUP)
        ustream_state_change(cl->us);
}

#endif
