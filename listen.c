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
 * File:   listen.c
 * Created on May 9, 2014, 5:28 PM
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <stdbool.h>

#include "listen.h"
#include "uhttpd.h"
#include "client.h"
#include "config.h"

/**
 * Listener structure
 */
struct listener {
    struct list_head list;      /* The listener list */
    struct uloop_fd fd;         /* Event loop information */
    int socket;                 /* The socket */
    int n_clients;              /* The number of clients */
    struct sockaddr_in6 addr;   /* The IPv6 socket address */
    bool tls;                   /* Flag for SSL support */
    bool blocked;               /* True if this listener is blocked */
};

/* The list of listeners */
static LIST_HEAD(listeners);

/* The number of blocked clients */
static int n_blocked;

/**
 * Close all listening sockets
 */
void close_listeners(void) {
    struct listener *l;

    /* Close all sockets in the listeners */
    list_for_each_entry(l, &listeners, list)
    close(l->fd.fd);
}

/**
 * If there is room for new connections unblock them
 * until the queue is full again.
 */
static void uh_poll_listeners(struct uloop_timeout *timeout) {
    struct listener *l;

    /* Check if there is room for new connections */
    if ((!n_blocked && conf->max_connections) || n_clients >= conf->max_connections) {
        return;
    }

    /* For each blocked listener, attach READ events*/
    list_for_each_entry(l, &listeners, list) {
        if (l->blocked) {

            /* Check if it can be unblocked */
            l->fd.cb(&l->fd, ULOOP_READ);
            if (n_clients >= conf->max_connections)
                break;

            /* Unblock listener */
            --n_blocked;
            l->blocked = false;
            uloop_fd_add(&l->fd, ULOOP_READ);
        }
    }
}

/**
 * Unblock all blocked listeners in the listener list.
 */
void unblock_listeners(void) {
    static struct uloop_timeout poll_timer = {
        .cb = uh_poll_listeners
    };

    uloop_timeout_set(&poll_timer, 1);
}

/**
 * This function handles new connections
 */
static void new_client_event(struct uloop_fd *fd, unsigned int events) {
    /* Get the listener that raised the event */
    struct listener *l = container_of(fd, struct listener, fd);

    /* Accept all clients */
    while (1) {
        if (!accept_client(fd->fd, l->tls))
            break;
    }

    /* Block a client when there are to many connections */
    if (conf->max_connections && n_clients >= conf->max_connections) {
        uloop_fd_delete(&l->fd);
        n_blocked++;
        l->blocked = true;
    }
}

/**
 * Setup all listeners in the listener list and
 * bind them to the uloop event system
 */
void setup_listeners(void) {
    struct listener *l;
    int yes = 1;

    /* For all listeners in the list */
    list_for_each_entry(l, &listeners, list) {
        int sock = l->fd.fd;

        /* Set up TCP Keep Alive for Linux */
        if (conf->keep_alive_time > 0) {
            int tcp_ka_idl, tcp_ka_int, tcp_ka_cnt;

            tcp_ka_idl = 1;
            tcp_ka_cnt = 3;
            tcp_ka_int = conf->keep_alive_time;

            setsockopt(sock, SOL_TCP, TCP_KEEPIDLE, &tcp_ka_idl, sizeof (tcp_ka_idl));
            setsockopt(sock, SOL_TCP, TCP_KEEPINTVL, &tcp_ka_int, sizeof (tcp_ka_int));
            setsockopt(sock, SOL_TCP, TCP_KEEPCNT, &tcp_ka_cnt, sizeof (tcp_ka_cnt));
            setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof (yes));
        }

        /* Register this listener with the uloop event loop and register READ events */
        l->fd.cb = new_client_event;
        uloop_fd_add(&l->fd, ULOOP_READ);
    }
}

/**
 * Bind a socket to listen from request on a given host on a given host.
 * @host the host to bind the socket to, NULL for any host
 * @port the port to listen to
 * @tls true if this socket sould listen for TLS connections
 */
bool bind_listener_sockets(const char *host, const char *port, bool tls) {
    int sock = -1;
    int yes = 1;
    int status;

    struct listener *l = NULL;
    struct addrinfo *addrs = NULL, *p = NULL;

    /*
     * Create POSIX hints for the address
     */
    static struct addrinfo hints = {
        .ai_family = AF_UNSPEC, /* The adress family is unspecified */
        .ai_socktype = SOCK_STREAM, /* This is a TCP socket */
        .ai_flags = AI_PASSIVE, /* The socket address will be used in a call to the bind function */
    };

    /* Translate the host and port into a set of socket addresses*/
    if ((status = getaddrinfo(host, port, &hints, &addrs)) != 0) {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(status));
        return false;
    }

    /* Try to bind a socket to each address returned from the translation*/
    for (p = addrs; p; p = p->ai_next) {
        /* Create the socket */
        sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock < 0) {
            perror("socket()");
            goto error;
        }

        /* Check if the address is not allready in use */
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (yes))) {
            perror("setsockopt()");
            goto error;
        }

        /* Required to get parallel v4 + v6 working */
        if (p->ai_family == AF_INET6 &&
                setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof (yes)) < 0) {
            perror("setsockopt()");
            goto error;
        }

        /* Bind the socket to the port and address */
        if (bind(sock, p->ai_addr, p->ai_addrlen) < 0) {
            perror("bind()");
            goto error;
        }

        /* Make a server socket  */
        if (listen(sock, UH_LIMIT_CLIENTS) < 0) {
            perror("listen()");
            goto error;
        }

        /* Change the file discriptor of the socket */
        fd_cloexec(sock);

        /* Reserver memory space for the listener */
        l = calloc(1, sizeof (*l));
        if (!l) {
            goto error;
        }

        /* Save the socket and TLS flag in the listener and append it to the listenerlist */
        l->fd.fd = sock;
        l->tls = tls;
        list_add_tail(&l->list, &listeners);

        /* Continue the loop when no error occured */
        continue;
error:
        /* Close socket if it was allready open */
        if (sock > -1) {
            close(sock);
        }
        return false;
    }

    /* Free the address list */
    freeaddrinfo(addrs);
    return true;
}
