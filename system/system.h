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
 * File:   system.h
 * Created on June 23, 2014, 5:17 PM
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <stdbool.h>

/* USB disk connection states */
#define USB_DISK_NOT_INSTALLED  "notinstalled"
#define USB_DISK_NOT_MOUNTED	"notmounted"
#define USB_DISK_MOUNTED	"mounted"

/*
 * Get the system hostname
 * @return the system hostname or NULL when an error occurred.
 */
char* system_get_hostname();

/*
 * Get the system model.
 * @return the system model or NULL when an error occurred.
 */
char* system_get_model();

/*
 * Returns true when a cable is physically connected to
 * the device, false otherwhise. When a bad iface is given
 * false will be returned.
 * @iface, the number from the interface to check. For the DPT
 * OpenWRT development boad this is '1' for WAN and '0'
 * for LAN.
 */
bool system_is_eth_connected(int port);

/*
 * Returns the USB disk state:
 *   - USB_DISK_NOT_INSTALLED: no physical device is connected
 *   - USB_DISK_NOT_MOUNTED: a device is connected but not mounted on /mnt
 *   - USB_DISK_MOUNTED: the device is mounted on /mnt
 */
const char* system_get_usb_storage_connection_state();

/*
 * Returns the free storage space on the usb
 * storage device when it is mounted.
 */
int system_get_usb_storage_freespace();

/*
 * Returns the total storage space on the usb
 * storage device when it is mounted on '/mnt'
 */
int system_get_usb_storage_totalspace();

/*
 * Get the current system load in linux style
 */
char* system_get_system_load();

/*
 * Get the free space in KiB of system RAM
 */
int system_get_ram_freespace();

/*
 * Get the total size in KiB of system RAM
 */
int system_get_ram_totalspace();

#endif /* SYSTEM_H_ */
