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
 * File:   nl80211.c
 * Created on May 18, 2015, 3:10 PM
 */

#include <stdbool.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include <ctype.h>
#include <net/if.h>

#include "nl.h"
#include "nl80211.h"
#include "../config.h"
#include "../logger.h"
#include "wifi.h"

/**
 * Map used to print security types 
 */
char* nl_sec_map[6] = {
    "none",
    "WEP",
    "WPA-personal",
    "WPA2-personal",
    "WPA-enterprise",
    "WPA2-enterprise"
};

/**
 * OUI information
 */
static unsigned char ms_oui[3] = { 
    0x00, 
    0x50, 
    0xf2 
};

/**
 * OUI information
 */
static unsigned char ieee80211_oui[3]= { 
    0x00, 
    0x0f, 
    0xac 
};

/**
 * The in-memory WiFi network list
 */
struct nl_wifi_network_list wifi_list = {
    .editing = false,
    .list = NULL
};

/**
 * Netlink socket error handler.
 */
static int _nl_err_handler(struct sockaddr_nl *addr, struct nlmsgerr *err, void *arg) {
    log_message(LOG_ERROR, "Netlink error handler was called: error %d\r\n", err->error);
    *((int*) arg) = err->error;
    return NL_STOP;
}

/**
 * Netlink socket finish action handler.
 */
static int _nl_finish_handler(struct nl_msg *msg, void *arg) {
    *((int*) arg) = 0;
    return NL_SKIP;
}

/**
 * Netlink socket finish action handler for scan dump
 */
static int _nl_dump_finish_handler(struct nl_msg *msg, void *arg) {
    *((int*) arg) = 0;
    nl_wifi_list_stop_edit(&wifi_list);
    return NL_SKIP;
}

/**
 * Netlink socket acknowledge action handler
 */
static int _nl_ack_handler(struct nl_msg *msg, void *arg) {
    // Callback for NL_CB_ACK.
    *((int*) arg) = 0;
    return NL_STOP;
}

/**
 * Netlink socket family handler
 */
static int _nl_family_handler(struct nl_msg *msg, void *arg)
{
    struct fmly_handler_args *grp = arg;
    struct nlattr *tb[CTRL_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *mcgrp;
    int rem_mcgrp;

    nla_parse(tb, CTRL_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

    if (!tb[CTRL_ATTR_MCAST_GROUPS])
        return NL_SKIP;

    nla_for_each_nested(mcgrp, tb[CTRL_ATTR_MCAST_GROUPS], rem_mcgrp) {
        struct nlattr *tb_mcgrp[CTRL_ATTR_MCAST_GRP_MAX + 1];

        nla_parse(tb_mcgrp, CTRL_ATTR_MCAST_GRP_MAX, nla_data(mcgrp), nla_len(mcgrp), NULL);

        if (!tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME] ||
            !tb_mcgrp[CTRL_ATTR_MCAST_GRP_ID])
                continue;
        if (strncmp(nla_data(tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME]),
                    grp->group, nla_len(tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME])))
                continue;
        grp->id = nla_get_u32(tb_mcgrp[CTRL_ATTR_MCAST_GRP_ID]);
        break;
    }

    return NL_SKIP;
}

/**
 * Netlink socket no sequence checking handler
 */
static int _nl_no_seq_check_handler(struct nl_msg *msg, void *arg)
{
    return NL_OK;
}

/**
 * Static helper function to get the multicast ID of a message family. 
 * @param sock the Netlink socket.
 * @param family the message family.
 * @param group the message group. 
 * @return the multicast ID or error code when negative.
 */
static int _nl_get_multicast_id(struct nl_sock *sock, const char *family, const char *group)
{
    struct nl_msg *msg;
    struct nl_cb *cb;
    int ret, ctrlid;
    struct fmly_handler_args grp = {
        .group = group,
        .id = -1,
    };

    msg = nlmsg_alloc();
    if (!msg) {
        return -1;
    }

    cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb) {
        ret = -1;
        goto out_fail_cb;
    }

    ctrlid = genl_ctrl_resolve(sock, "nlctrl");

    genlmsg_put(msg, 0, 0, ctrlid, 0, 0, CTRL_CMD_GETFAMILY, 0);
    ret = -1;
    NLA_PUT_STRING(msg, CTRL_ATTR_FAMILY_NAME, family);

    ret = nl_send_auto_complete(sock, msg);
    if (ret < 0) {
        goto nla_put_failure;
    }
    ret = 1;

    nl_cb_err(cb, NL_CB_CUSTOM, _nl_err_handler, &ret);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, _nl_ack_handler, &ret);
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, _nl_family_handler, &grp);

    while (ret > 0) {
        nl_recvmsgs(sock, cb);
    }

    if (ret == 0) {
        ret = grp.id;
    }
        
 nla_put_failure:
    nl_cb_put(cb);
 
 out_fail_cb:
    nlmsg_free(msg);
    return ret;
}

/**
 * Netlink wait event handler.
 * @param msg the Netlink message.
 * @param arg the handler arguments.
 * @return the Netlink action.
 */
static int _nl_wait_event(struct nl_msg *msg, void *arg)
{
    struct wait_event *wait = arg;
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    int i;

    for (i = 0; i < wait->n_cmds; i++) {
        if (gnlh->cmd == wait->cmds[i]) {
            wait->cmd = gnlh->cmd;
        }
    }

    return NL_SKIP;
}

/**
 * Listen for events from multicast groups.
 * @param state the Netlink socket state.
 * @param n_waits the number of waits. 
 * @param waits the time to wait. 
 * @return the event command received. 
 */
static unsigned int _nl_listen_events(struct nl_state *state, const int n_waits, const unsigned int *waits)
{
    int mcid, ret;
    struct wait_event wait_ev;
    struct nl_cb *cb;

    /* Add socket to scan multicast group of the nl80211 family */
    mcid = _nl_get_multicast_id(state->nl_sock, "nl80211", "scan");
    if (mcid >= 0) {
        ret = nl_socket_add_membership(state->nl_sock, mcid);
        if(ret) {
            return ret;
        }
    }
    
    cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb) {
        log_message(LOG_ERROR, "Failed to allocate netlink callbacks\r\n");
        return -1;
    }

    nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, _nl_no_seq_check_handler, NULL);
    if (n_waits && waits) {
        wait_ev.cmds = waits;
        wait_ev.n_cmds = n_waits;
        nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, _nl_wait_event, &wait_ev);
    }

    wait_ev.cmd = 0;
    while (!wait_ev.cmd) {
        nl_recvmsgs(state->nl_sock, cb);
    }

    nl_cb_put(cb);
    return wait_ev.cmd;
}

/**
 * Initializer for the the Netlink connection.
 * @param state the Netlink connection state struct.
 * @return true on success, false on error.
 */
bool nl_init(struct nl_state *state)
{
    state->nl_sock = nl_socket_alloc();
    
    /* Create a netlink socket */
    if(!state->nl_sock) {
        log_message(LOG_ERROR, "Could not create netlink socket\r\n");
        return false;
    }
    
    /* Connect to netlink socket */
    nl_socket_set_buffer_size(state->nl_sock, 8192, 8192);
    if(genl_connect(state->nl_sock)) {
        log_message(LOG_ERROR, "Could not connect to netlink socket\r\n");
        nl_socket_free(state->nl_sock);
        return false;
    }
    
    state->nl80211_id = genl_ctrl_resolve(state->nl_sock, "nl80211");
    if(state->nl80211_id < 0) {
        log_message(LOG_ERROR, "Could not find nl80211 kernel driver\r\n");
        nl_socket_free(state->nl_sock);
        return false;
    }
    
    /* Get WLAN device ID */
    state->wlan_devid = if_nametoindex(WLAN_DEVICE);
    if(state->wlan_devid <= 0) {
        log_message(LOG_WARNING, "Wireless network adapter '%s' not detected, trying alternative\r\n", WLAN_DEVICE);
        state->wlan_devid = if_nametoindex(WLAN_DEVICE_ALT);
        if(state->wlan_devid <= 0) {
            log_message(LOG_ERROR, "Wireless network adapter alternative '%s' not detected\r\n", WLAN_DEVICE_ALT);
            return false;
        } else {
            return true;
        }
    }
    
    return true;
}

/**
 * Clean the Netlink connection state.
 * @param state the Netlink connection state struct.
 */
void nl_clean(struct nl_state *state)
{
    nl_socket_free(state->nl_sock);
}

/**
 * Trigger a WLAN scan on the default WLAN adapter.
 * @param nlstate the Netlink connection state.
 * @return true on success, false on error. 
 */
bool nl_trigger_scan(struct nl_state *nlstate)
{
    struct nl_msg *msg = NULL;
    struct nl_msg *ssids = NULL;
    struct nl_cb *cb = NULL;
    int err;
    int ret;
    
    static const unsigned int cmds[] = {
        NL80211_CMD_NEW_SCAN_RESULTS,
        NL80211_CMD_SCAN_ABORTED,
    };
    
    /* Create Netlink message structure for Netlink commands */
    msg = nlmsg_alloc();
    if(!msg) {
        log_message(LOG_ERROR, "Could not create netlink message structures\r\n");
        return false;
    }
    
    /* Create psuedo SSID's message structure */
    ssids = nlmsg_alloc();
    if(!ssids) {
        log_message(LOG_ERROR, "Could not create ssids netlink message structure\r\n");
        return false;
    }
    
    /* Create Netlink callback */
    cb = nl_cb_alloc(NL_CB_DEFAULT);
    if(!cb) {
        log_message(LOG_ERROR, "Could not create netlink callback\r\n");
        goto nla_put_failure;
    }
    
    /* Fill up kernel command */
    genlmsg_put(msg, 0, 0, nlstate->nl80211_id, 0, 0, NL80211_CMD_TRIGGER_SCAN, 0);     
    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, nlstate->wlan_devid);
    nla_put(ssids, 1, 0, "");  
    nla_put_nested(msg, NL80211_ATTR_SCAN_SSIDS, ssids);
    nlmsg_free(ssids);
    nl_cb_err(cb, NL_CB_CUSTOM, _nl_err_handler, &err);                                       
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, _nl_finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, _nl_ack_handler, &err);
    
    /* Send the command and active wait for answer */
    err = 1;
    ret = nl_send_auto_complete(nlstate->nl_sock, msg);

    while(err > 0) {
        ret = nl_recvmsgs(nlstate->nl_sock, cb);
    }
    if(err < 0) {
        log_message(LOG_ERROR, "WiFi scan trigger produced error: %d\r\n", err);
        goto nla_put_failure;
    }
    if(ret < 0) {
        log_message(LOG_ERROR, "WiFi scan trigger produced error: %d (%s)\r\n", ret, nl_geterror(-ret));
        goto nla_put_failure;
    }
    
    /* Now listen for the scan finish event */
    if (_nl_listen_events(nlstate, 2, cmds) == NL80211_CMD_SCAN_ABORTED) {
        log_message(LOG_ERROR, "WiFi scan was aborted by the kernel\r\n");
        return false;
    }
    
    nlmsg_free(msg);
    return true;

nla_put_failure:
    nlmsg_free(msg);
    return false;
}

/**
 * Helper function to decode the SSID of the network probe/beacon packet. 
 * @param buf the destination buffer. 
 * @param ie the beacon packet information elements.
 * @param ielen the information elements length. 
 */
static void _nl_decode_ssid(char* buf, unsigned char *ie, int ielen) {
    uint8_t len;
    uint8_t *data;
    int i;
    int cursor = 0;

    while (ielen >= 2 && ielen >= ie[1]) {
        if (ie[0] == 0 && ie[1] >= 0 && ie[1] <= 32) {
            len = ie[1];
            data = ie + 2;
            for (i = 0; i < len; i++) {
                if (isprint(data[i]) && data[i] != ' ' && data[i] != '\\') {
                    cursor += snprintf(buf+cursor, 32-cursor, "%c", data[i]);
                } else if (data[i] == ' ' && (i != 0 && i != len -1)) {
                    cursor += snprintf(buf+cursor, 32-cursor, " ");
                } else {
                    cursor += snprintf(buf+cursor, 32-cursor, "\\x%.2x", data[i]);
                }
            }
            break;
        }
        ielen -= ie[1] + 2;
        ie += ie[1] + 2;
    }
}

/**
 * Given the WPA(2) data element, returns true if 
 * this is a WPA(2) enterprise network or not. If PSK
 * is used, it is WPA personal. 
 * @param data the correct data element in the RSN or WPA IE. 
 * @return true if this is enterprise WiFi. 
 */
static bool is_enterprise(uint8_t *data) 
{
    unsigned char* d = data + 8;
    unsigned char l = data[1] - 6;
    uint16_t count = d[0] | (d[1] << 8);

    if (2 + (count * 4) <= l) {
        d += 2 + (count * 4);
        l -= 2 + (count * 4);

        if (l < 2) {
            /* This is IEEE 802.1X, thus enterprise */
            return true;
        } else {
            count = d[0] | (d[1] << 8);
            if (2 + (count * 4) <= l){
                d += 2;

                if (memcmp(d, ms_oui, 3) == 0) {
                    switch (d[3]) {
                        case 2:     /* PSK */   
                            return false;

                        case 1:     /* IEEE 802.1X") */
                        default:    /* Proprietary */
                            return true;
                    }
                } else if (memcmp(d, ieee80211_oui, 3) == 0) {
                    switch (d[3]) {
                        case 2:     /* PSK */
                        case 4:     /* FT/PSK */
                            return false;

                        case 1:     /* IEEE 802.1X */
                        case 3:     /* FT/IEEE 802.1X */
                        case 5:     /* IEEE 802.1X/SHA-256 */
                        case 6:     /* PSK/SHA-256 */
                        case 7:     /* TDLS/TPK */
                        default:    /* Proprietary */
                            return true;
                    }
                } else {
                    /* Proprietary */
                    return true;
                }     
            }
        }
    }
    
    return false;
}

/**
 * This callback function is called for every SSID when a WiFi scan
 * dump is requested from the kernel. 
 * @param msg the Netlink message structure. 
 * @param arg the callback function arguments. 
 * @return next action procedure. 
 */
static int _nl_ssid_callback(struct nl_msg *msg, void *arg) {

    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    struct nlattr *bss[NL80211_BSS_MAX + 1];
    static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
        [NL80211_BSS_TSF] = { .type = NLA_U64 },
        [NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
        [NL80211_BSS_BSSID] = { },
        [NL80211_BSS_BEACON_INTERVAL] = { .type = NLA_U16 },
        [NL80211_BSS_CAPABILITY] = { .type = NLA_U16 },
        [NL80211_BSS_INFORMATION_ELEMENTS] = { },
        [NL80211_BSS_SIGNAL_MBM] = { .type = NLA_U32 },
        [NL80211_BSS_SIGNAL_UNSPEC] = { .type = NLA_U8 },
        [NL80211_BSS_STATUS] = { .type = NLA_U32 },
        [NL80211_BSS_SEEN_MS_AGO] = { .type = NLA_U32 },
        [NL80211_BSS_BEACON_IES] = { },
    };

    /* Try to parse the BSS data */
    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);
    if(!tb[NL80211_ATTR_BSS]){
        log_message(LOG_ERROR, "The WiFi scan dump does not contain bss data\r\n");
        return NL_SKIP;
    }
    
    if(nla_parse_nested(bss, NL80211_BSS_MAX, tb[NL80211_ATTR_BSS], bss_policy)){
        log_message(LOG_ERROR, "Could not parse WiFi network information\r\n");
        return NL_SKIP;
    }
    
    if(!bss[NL80211_BSS_BSSID] || !bss[NL80211_BSS_INFORMATION_ELEMENTS]){
        return NL_SKIP;
    }

    /* Decode binary data and store in w_net struct */
    struct nl_wifi_network *w_net = (struct nl_wifi_network*) calloc(1, sizeof(struct nl_wifi_network));
    
    _nl_decode_ssid(w_net->ssid, nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]), nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]));
    if (bss[NL80211_BSS_SIGNAL_MBM]) {
        w_net->signal = nla_get_u32(bss[NL80211_BSS_SIGNAL_MBM]);
        w_net->signal /= 100;
    }
    
    /* Calculate quality */
    if(w_net->signal <= -100) {
        w_net->quality = 0;
    } else if (w_net->signal >= -50) {
        w_net->quality = 100;
    } else {
        w_net->quality = (2*(w_net->signal + 100));
    }
    
    /* Determine WiFi security level */
    unsigned char* data = NULL;
    int data_len = 0;
    w_net->security = NL_WIFI_SECURITY_NONE;

    /* Check for WEP */
    if (bss[NL80211_BSS_CAPABILITY]) {
        uint16_t capa = nla_get_u16(bss[NL80211_BSS_CAPABILITY]);
        if (capa & WLAN_CAPABILITY_PRIVACY) {
            w_net->security = NL_WIFI_SECURITY_WEP;
        }
    }
    
    /* Check for WPA/WPA2 */
    if(bss[NL80211_BSS_INFORMATION_ELEMENTS]) {
        data = nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]);
        data_len = nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]);
    } else if(bss[NL80211_BSS_BEACON_IES]) {
        data = nla_data(bss[NL80211_BSS_BEACON_IES]);
        data_len = nla_len(bss[NL80211_BSS_BEACON_IES]);
    }

    if(data != NULL) {
        while (data_len >= 2 && data_len >= data[1]) {
            if(data[0] == IE_RSN) {
                if(is_enterprise(data)) {
                    w_net->security = NL_WIFI_SECURITY_WPA2_E;
                } else {
                    w_net->security = NL_WIFI_SECURITY_WPA2_P;
                }
            } else if (data[0] == IE_WPA) {
                if(data[3] == 1) {
                    if(is_enterprise(data)) {
                        w_net->security = NL_WIFI_SECURITY_WPA_E;
                    } else {
                        w_net->security = NL_WIFI_SECURITY_WPA_P;
                    }
                }
            }
            data_len -= data[1] + 2;
            data += data[1] + 2;
	}
    }
    
    /* Add the network to the list */
    nl_wifi_list_add(&wifi_list, w_net);

    return NL_SKIP;
}

/**
 * Dump the WiFi scan results into memory.
 * @param nlstate the Netlink connection state.
 * @return true on success, false on error.
 */
bool nl_dump_scan(struct nl_state *nlstate)
{
    struct nl_msg *msg = NULL;
    struct nl_cb *cb = NULL;
    int err;
    int ret;
    
    /* Create Netlink message structure for Netlink commands */
    msg = nlmsg_alloc();
    if(!msg) {
        log_message(LOG_ERROR, "Could not create netlink message structures\r\n");
        return false;
    }
    
    /* Create Netlink callback */
    cb = nl_cb_alloc(NL_CB_DEFAULT);
    if(!cb) {
        log_message(LOG_ERROR, "Could not create netlink callback\r\n");
        goto nla_put_failure;
    }
    
    /* Fill up kernel command */
    genlmsg_put(msg, 0, 0, nlstate->nl80211_id, 0, NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);     
    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, nlstate->wlan_devid);
    nl_cb_err(cb, NL_CB_CUSTOM, _nl_err_handler, &err);                                       
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, _nl_dump_finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, _nl_ack_handler, &err);  
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, _nl_ssid_callback, NULL);
    
    /* Start editing the WiFi list */
    nl_wifi_start_edit(&wifi_list);    
    
    /* Send the command and active wait for answer */
    err = 1;
    ret = nl_send_auto_complete(nlstate->nl_sock, msg);

    while(err > 0) {
        ret = nl_recvmsgs(nlstate->nl_sock, cb);
    }
    if(err < 0) {
        log_message(LOG_ERROR, "WiFi scan dump produced error: %d\r\n", err);
        goto nla_put_failure;
    }
    if(ret < 0) {
        log_message(LOG_ERROR, "WiFi scan dump produced error: %d (%s)\r\n", ret, nl_geterror(-ret));
        goto nla_put_failure;
    }
 
    nlmsg_free(msg);
    return true;

nla_put_failure:
    nlmsg_free(msg);
    return false;
}

/**
 * Clear all entries in the WiFi network list and start editing.
 * @param w_list the list to clear.
 */
void nl_wifi_start_edit(struct nl_wifi_network_list *w_list)
{
    pthread_mutex_lock(&(w_list->lock));
    struct nl_wifi_network *w_cur = w_list->list;
    struct nl_wifi_network *w_next = NULL;
    
    while(w_cur != NULL) {
        w_next = w_cur->next;
        free(w_cur);
        w_cur = w_next;
    }
    
    w_list->list = NULL;
    w_list->editing = true;
    
    pthread_mutex_unlock(&(w_list->lock));
}

/**
 * Add a a WiFi network to the list.
 * @param w_list the list to add the WiFi network to.
 * @param w_net the network to add. 
 */
void nl_wifi_list_add(struct nl_wifi_network_list *w_list, struct nl_wifi_network *w_net)
{
    pthread_mutex_lock(&(w_list->lock));
    
    if(w_list->list == NULL) {
        w_list->list = w_net;
    } else {
        struct nl_wifi_network *w_cur = w_list->list;

        while(w_cur->next != NULL) {
            w_cur = w_cur->next;
        }

        w_cur->next = w_net;
        w_net->next = NULL;
    }

    pthread_mutex_unlock(&(w_list->lock));
}

/**
 * Turn off the editing flag.
 * @param w_list the list to stop editing.
 */
void nl_wifi_list_stop_edit(struct nl_wifi_network_list *w_list)
{
    pthread_mutex_lock(&(w_list->lock));
    w_list->editing = false;
    pthread_mutex_unlock(&(w_list->lock));    
}

/**
 * Scan for WiFi networks in reach without shutting down WiFi connection
 * @return true on success, false on error. 
 */
bool nl_scan_networks()
{
    struct nl_state nlstate;
    
    /* Initialize Netlink kernel connection */
    if(!nl_init(&nlstate)) {
        log_message(LOG_ERROR, "Could not create Netlink socket\r\n");
        return false;
    }
    
    /* Trigger a wireless scan */
    if(!nl_trigger_scan(&nlstate)) {
        log_message(LOG_ERROR, "Could not trigger WiFi scan\r\n");
        nl_clean(&nlstate);
        return false;
    }
   
    /* Read the scan results */
    if(!nl_dump_scan(&nlstate)) {
        log_message(LOG_ERROR, "Could not read WiFi scan results\r\n");
        nl_clean(&nlstate);
        return false;
    }
    
    nl_clean(&nlstate);
    return true;
}
