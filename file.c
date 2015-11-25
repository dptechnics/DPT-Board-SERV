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

#define _BSD_SOURCE
#define _DARWIN_C_SOURCE
#define _XOPEN_SOURCE 700

#include <sys/types.h>
#include <sys/dir.h>
#include <time.h>
#include <strings.h>
#include <dirent.h>

#include <libubox/blobmsg.h>

#include "uhttpd.h"
#include "mimetypes.h"
#include "client.h"
#include "config.h"
#include "api.h"
#include "logger.h"

/* Pending HTTP requests */
static LIST_HEAD(pending_requests);

struct deferred_request {
    struct list_head list;
    struct dispatch_handler *d;
    struct client *cl;
    struct path_info pi;
    bool called, path;
};

enum file_hdr {
    HDR_AUTHORIZATION,
    HDR_IF_MODIFIED_SINCE,
    HDR_IF_UNMODIFIED_SINCE,
    HDR_IF_MATCH,
    HDR_IF_NONE_MATCH,
    HDR_IF_RANGE,
    __HDR_MAX
};

/**
 * Try to normalize the a path to a canonical path
 */
static char * canonpath(const char *path, char *path_resolved) {
    const char *path_cpy = path;
    char *path_res = path_resolved;

    if (conf->no_symlinks)
        return realpath(path, path_resolved);

    /* normalize */
    while ((*path_cpy != '\0') && (path_cpy < (path + PATH_MAX - 2))) {
        if (*path_cpy != '/')
            goto next;

        /* skip repeating / */
        if (path_cpy[1] == '/') {
            path_cpy++;
            continue;
        }

        /* /./ or /../ */
        if (path_cpy[1] == '.') {
            /* skip /./ */
            if ((path_cpy[2] == '/') || (path_cpy[2] == '\0')) {
                path_cpy += 2;
                continue;
            }

            /* collapse /x/../ */
            if ((path_cpy[2] == '.') &&
                    ((path_cpy[3] == '/') || (path_cpy[3] == '\0'))) {
                while ((path_res > path_resolved) && (*--path_res != '/'));

                path_cpy += 3;
                continue;
            }
        }

next:
        *path_res++ = *path_cpy++;
    }

    /* remove trailing slash if not root / */
    if ((path_res > (path_resolved + 1)) && (path_res[-1] == '/'))
        path_res--;
    else if (path_res == path_resolved)
        *path_res++ = '/';

    *path_res = '\0';

    return path_resolved;
}

/**
 * Given a url this functions tries to find the physical path on the server.
 * @cl the client that made the request
 * @url the requested URL
 * @return NULL on error
 */
static struct path_info *path_lookup(struct client *cl, const char *url) {
    static char path_phys[PATH_MAX];
    static char path_info[PATH_MAX];
    static struct path_info p;

    int docroot_len = strlen(conf->document_root);
    char *pathptr = NULL;
    bool slash;

    int i = 0;
    int len;
    struct stat s;

    /* Return NULL when the URL is undefined */
    if (url == NULL)
        return NULL;

    memset(&p, 0, sizeof (p));
    path_phys[0] = 0;
    path_info[0] = 0;

    /* Start the canonical path with the document root */
    strcpy(uh_buf, conf->document_root);

    /* Separate query string from url */
    if ((pathptr = strchr(url, '?')) != NULL) {
        p.query = pathptr[1] ? pathptr + 1 : NULL;

        /* URL decode component without query */
        if (pathptr > url) {
            if (uh_urldecode(&uh_buf[docroot_len],
                    sizeof (uh_buf) - docroot_len - 1,
                    url, pathptr - url) < 0)
                return NULL;

        }
    }
    
    /* Decode the full url when  there is no querystring */
    else if (uh_urldecode(&uh_buf[docroot_len],
            sizeof (uh_buf) - docroot_len - 1,
            url, strlen(url)) < 0)
        return NULL;

    /* Create canonical path */
    len = strlen(uh_buf);
    slash = len && uh_buf[len - 1] == '/';
    len = min(len, sizeof (path_phys) - 1);

    for (i = len; i >= 0; i--) {
        char ch = uh_buf[i];
        bool exists;

        if (ch != 0 && ch != '/')
            continue;

        uh_buf[i] = 0;
        exists = !!canonpath(uh_buf, path_phys);
        uh_buf[i] = ch;

        if (!exists)
            continue;

        /* Test the current path */
        if (stat(path_phys, &p.stat))
            continue;

        snprintf(path_info, sizeof (path_info), "%s", uh_buf + i);
        break;
    }

    /* Check whether found path is within docroot */
    if (strncmp(path_phys, conf->document_root, docroot_len) != 0 ||
            (path_phys[docroot_len] != 0 &&
            path_phys[docroot_len] != '/')) {
        return NULL;
    }

    /* Check if the found file is a regular file */
    if (p.stat.st_mode & S_IFREG) {
        p.root = conf->document_root;
        p.phys = path_phys;
        p.name = &path_phys[docroot_len];
        p.info = path_info[0] ? path_info : NULL;
        return &p;
    }

    /* Make sure it is not a directory */
    if (!(p.stat.st_mode & S_IFDIR)) {
        return NULL;
    }

    if (path_info[0]) {
        return NULL;
    }

    pathptr = path_phys + strlen(path_phys);

    /* ensure trailing slash */
    if (pathptr[-1] != '/') {
        pathptr[0] = '/';
        pathptr[1] = 0;
        pathptr++;
    }

    /* if requested url resolves to a directory and a trailing slash
       is missing in the request url, redirect the client to the same
       url with trailing slash appended */
    if (!slash) {
        write_http_header(cl, 302, "Found");
        ustream_printf(cl->us, "Content-Length: 0\r\n");
        ustream_printf(cl->us, "Location: %s%s%s\r\n\r\n",
                &path_phys[docroot_len],
                p.query ? "?" : "",
                p.query ? p.query : "");
        request_done(cl);
        p.redirected = 1;
        return &p;
    }

    /* Check if the folder contains an index file */
    len = path_phys + sizeof (path_phys) - pathptr - 1;
    if (strlen(conf->index_file) <= len) {

        strcpy(pathptr, conf->index_file);
        if (!stat(path_phys, &s) && (s.st_mode & S_IFREG)) {
            memcpy(&p.stat, &s, sizeof (p.stat));
        } else {
            /* Stop when strcpy is not needed */
            *pathptr = 0;
        }
    }

    p.root = conf->document_root;
    p.phys = path_phys;
    p.name = &path_phys[docroot_len];

    return p.phys ? &p : NULL;
}

/**
 * Lookup the mimetype of a file based on the file extension
 * @path the full filepath
 * @return "application/octet-stream" when a mimetype could not be found
 */
static const char * file_mime_lookup(const char *path) {
    const struct mimetype *m = &uh_mime_types[0];
    const char *e;

    while (m->extn) {
        e = &path[strlen(path) - 1];

        while (e >= path) {
            if ((*e == '.' || *e == '/') && !strcasecmp(&e[1], m->extn))
                return m->mime;
            --e;
        }
        ++m;
    }

    return "application/octet-stream";
}

/**
 * Create an etag for the file
 */
static const char * make_file_etag(struct stat *s, char *buf, int len) {
    snprintf(buf, len, "\"%x-%x-%x\"",
            (unsigned int) s->st_ino,
            (unsigned int) s->st_size,
            (unsigned int) s->st_mtime);

    return buf;
}

static time_t uh_file_date2unix(const char *date) {
    struct tm t;

    memset(&t, 0, sizeof (t));

    if (strptime(date, "%a, %d %b %Y %H:%M:%S %Z", &t) != NULL)
        return timegm(&t);

    return 0;
}

static char * uh_file_unix2date(time_t ts, char *buf, int len) {
    struct tm *t = gmtime(&ts);

    strftime(buf, len, "%a, %d %b %Y %H:%M:%S GMT", t);

    return buf;
}

static char *uh_file_header(struct client *cl, int idx) {
    if (!cl->dispatch.file.hdr[idx])
        return NULL;

    return (char *) blobmsg_data(cl->dispatch.file.hdr[idx]);
}

static void uh_file_response_ok_hdrs(struct client *cl, struct stat *s) {
    char buf[128];

    if (s) {
        ustream_printf(cl->us, "ETag: %s\r\n", make_file_etag(s, buf, sizeof (buf)));
        ustream_printf(cl->us, "Last-Modified: %s\r\n",
                uh_file_unix2date(s->st_mtime, buf, sizeof (buf)));
    }
    ustream_printf(cl->us, "Date: %s\r\n",
            uh_file_unix2date(time(NULL), buf, sizeof (buf)));
}

static void uh_file_response_200(struct client *cl, struct stat *s) {
    write_http_header(cl, 200, "OK");
    return uh_file_response_ok_hdrs(cl, s);
}

static void uh_file_response_304(struct client *cl, struct stat *s) {
    write_http_header(cl, 304, "Not Modified");

    return uh_file_response_ok_hdrs(cl, s);
}

static void uh_file_response_412(struct client *cl) {
    write_http_header(cl, 412, "Precondition Failed");
}

static bool uh_file_if_match(struct client *cl, struct stat *s) {
    char buf[128];
    const char *tag = make_file_etag(s, buf, sizeof (buf));
    char *hdr = uh_file_header(cl, HDR_IF_MATCH);
    char *p;
    int i;

    if (!hdr)
        return true;

    p = &hdr[0];
    for (i = 0; i < strlen(hdr); i++) {
        if ((hdr[i] == ' ') || (hdr[i] == ',')) {
            hdr[i++] = 0;
            p = &hdr[i];
        } else if (!strcmp(p, "*") || !strcmp(p, tag)) {
            return true;
        }
    }

    uh_file_response_412(cl);
    return false;
}

static int uh_file_if_modified_since(struct client *cl, struct stat *s) {
    char *hdr = uh_file_header(cl, HDR_IF_MODIFIED_SINCE);

    if (!hdr)
        return true;

    if (uh_file_date2unix(hdr) >= s->st_mtime) {
        uh_file_response_304(cl, s);
        return false;
    }

    return true;
}

static int uh_file_if_none_match(struct client *cl, struct stat *s) {
    char buf[128];
    const char *tag = make_file_etag(s, buf, sizeof (buf));
    char *hdr = uh_file_header(cl, HDR_IF_NONE_MATCH);
    char *p;
    int i;

    if (!hdr)
        return true;

    p = &hdr[0];
    for (i = 0; i < strlen(hdr); i++) {
        if ((hdr[i] == ' ') || (hdr[i] == ',')) {
            hdr[i++] = 0;
            p = &hdr[i];
        } else if (!strcmp(p, "*") || !strcmp(p, tag)) {
            if ((cl->request.method == UH_HTTP_MSG_GET) ||
                    (cl->request.method == UH_HTTP_MSG_HEAD))
                uh_file_response_304(cl, s);
            else
                uh_file_response_412(cl);

            return false;
        }
    }

    return true;
}

static int uh_file_if_range(struct client *cl, struct stat *s) {
    char *hdr = uh_file_header(cl, HDR_IF_RANGE);

    if (hdr) {
        uh_file_response_412(cl);
        return false;
    }

    return true;
}

static int uh_file_if_unmodified_since(struct client *cl, struct stat *s) {
    char *hdr = uh_file_header(cl, HDR_IF_UNMODIFIED_SINCE);

    if (hdr && uh_file_date2unix(hdr) <= s->st_mtime) {
        uh_file_response_412(cl);
        return false;
    }

    return true;
}

static void file_write_cb(struct client *cl) {
    int fd = cl->dispatch.file.fd;
    int r;

    while (1) {
        r = read(fd, uh_buf, sizeof (uh_buf));
        if (r < 0) {
            if (errno == EINTR)
                continue;
        }

        if (!r) {
            request_done(cl);
            return;
        }

        /* 
         *(uh_buf + r) = '\0'; 
        ustream_printf(cl->us, "%s", uh_buf);
         */
        ustream_write(cl->us, uh_buf, r, true);

    }
}

static void uh_file_free(struct client *cl) {
    close(cl->dispatch.file.fd);
}

static void uh_file_data(struct client *cl, struct path_info *pi, int fd) {
    /* test preconditions */
    if (!uh_file_if_modified_since(cl, &pi->stat) ||
            !uh_file_if_match(cl, &pi->stat) ||
            !uh_file_if_range(cl, &pi->stat) ||
            !uh_file_if_unmodified_since(cl, &pi->stat) ||
            !uh_file_if_none_match(cl, &pi->stat)) {
        ustream_printf(cl->us, "Content-Length: 0\r\n");
        ustream_printf(cl->us, "\r\n");
        request_done(cl);
        close(fd);
        return;
    }

    /* write status */
    uh_file_response_200(cl, &pi->stat);

    ustream_printf(cl->us, "Content-Type: %s\r\n",
            file_mime_lookup(pi->name));

    /* Don't use content-length when chunked encoding */
    if (!uh_use_chunked(cl)) {
        ustream_printf(cl->us, "Content-Length: %i\r\n\r\n", pi->stat.st_size);
    }


    /* Stop if this is a header only request */
    if (cl->request.method == UH_HTTP_MSG_HEAD) {
        request_done(cl);
        close(fd);
        return;
    }

    cl->dispatch.file.fd = fd;
    cl->dispatch.write_cb = file_write_cb;
    cl->dispatch.free = uh_file_free;
    cl->dispatch.close_fds = uh_file_free;
    file_write_cb(cl);
}

static void uh_file_request(struct client *cl, const char *url, struct path_info *pi, struct blob_attr **tb) {
    int fd;

    if (!(pi->stat.st_mode & S_IROTH))
        goto error;

    if (pi->stat.st_mode & S_IFREG) {
        fd = open(pi->phys, O_RDONLY);
        if (fd < 0)
            goto error;

        cl->dispatch.file.hdr = tb;
        uh_file_data(cl, pi, fd);
        cl->dispatch.file.hdr = NULL;
        return;
    }

    if ((pi->stat.st_mode & S_IFDIR)) {
        goto error;
    }

error:
    client_send_error(cl, 403, "Forbidden", "You don't have permission to access %s on this server.", url);
}

static bool handle_file_request(struct client *cl, char *url) {

    static const struct blobmsg_policy hdr_policy[__HDR_MAX] = {
        [HDR_AUTHORIZATION] =
        { "authorization", BLOBMSG_TYPE_STRING},
        [HDR_IF_MODIFIED_SINCE] =
        { "if-modified-since", BLOBMSG_TYPE_STRING},
        [HDR_IF_UNMODIFIED_SINCE] =
        { "if-unmodified-since", BLOBMSG_TYPE_STRING},
        [HDR_IF_MATCH] =
        { "if-match", BLOBMSG_TYPE_STRING},
        [HDR_IF_NONE_MATCH] =
        { "if-none-match", BLOBMSG_TYPE_STRING},
        [HDR_IF_RANGE] =
        { "if-range", BLOBMSG_TYPE_STRING},
    };
    struct blob_attr * tb[__HDR_MAX];
    struct path_info *pi;

    pi = path_lookup(cl, url);
    if (!pi)
        return false;

    if (pi->redirected)
        return true;

    blobmsg_parse(hdr_policy, __HDR_MAX, tb, blob_data(cl->hdr.head), blob_len(cl->hdr.head));
    if (tb[HDR_AUTHORIZATION])
        pi->auth = blobmsg_data(tb[HDR_AUTHORIZATION]);

    /* Handle file request */
    uh_file_request(cl, url, pi, tb);

    return true;
}

void uh_handle_request(struct client *cl) {
    struct http_request *req = &cl->request;
    char *url = blobmsg_data(blob_data(cl->hdr.head));

    req->redirect_status = 200;

    /* Check if this is an api or file request */
    if (uh_path_match(conf->api_prefix, url)) {
        cl->use_chunked = false;
        api_handle_request(cl, url);
        return;
    } else {
        cl->use_chunked = false;
        if (handle_file_request(cl, url))
            return;
    }

    log_message(LOG_DEBUG, "A 404 is generated for request: '%s'\r\n", url);
    req->redirect_status = 404;
    client_send_error(cl, 404, "Not Found", "The requested URL %s was not found on this server.", url);
}
