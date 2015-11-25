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
 * File:   system.c
 * Created on June 23, 2014, 5:17 PM
 */

#include <mntent.h>
#include <sys/vfs.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/sysinfo.h>

#include "../uhttpd.h"
#include "system.h"


/*
 * Get the system hostname
 * @return the system hostname or NULL when an error occurred.
 */
char* system_get_hostname()
{
    char *hostname = (char*) malloc(25*sizeof(char));	/* The hostname */
    gethostname(hostname, 25);
    return hostname;
}

/*
 * Get the system model.
 * @return the system model or NULL when an error occurred.
 */
char* system_get_model()
{
	FILE* fd;			/* File descriptor */
	char *model = (char*) malloc(26*sizeof(char));	/* The system model, max 25 characters */

	/* Get the system model */
	fd = fopen("/tmp/sysinfo/model", "r");
	if(fd == NULL || fgets(model, 25, fd) == NULL || fclose(fd) != 0){
		return NULL;
	}

	return model;
}

/*
 * Returns true when a cable is physically connected to
 * the device, false otherwhise. When a bad iface is given
 * false will be returned.
 * @iface, the number from the interface to check. For the DPT
 * OpenWRT development boad this is '1' for WAN and '0'
 * for LAN.
 */
bool system_is_eth_connected(int port)
{
	int fd;				/* File descriptor port state */
	char buf[30];		/* Buffer for port path */
	bool state;			/* Character for reading the state */

	/* Make the GPIO port path */
	sprintf(buf, "/sys/class/net/eth%d/operstate", port);

	/* Try to open device state file */
	fd = open(buf, O_RDONLY);
	if(fd < 0) {
		/* The file could not be opened */
		return false;
	}

	/* Read the port state */
	if(read(fd, &buf, 1) < 0) {
		return false;
	}

	/* If the file starts with u (up) the interface is connected */
	state = buf[0] == 'u' ? true : false;

	/* Close the file */
	if(close(fd) < 0) {
		return false;
	}

	/* Return the state */
	return state;
}

/*
 * Returns the USB disk state:
 *   - USB_DISK_NOT_INSTALLED: no physical device is connected
 *   - USB_DISK_NOT_MOUNTED: a device is connected but not mounted on /mnt
 *   - USB_DISK_MOUNTED: the device is mounted on /mnt
 */
const char* system_get_usb_storage_connection_state()
{
	struct stat s;
	FILE *mounts = NULL;
	struct mntent *mount_info;
	bool is_mounted = false;

	/* Check if there is a sda or sdb device in '/dev' */
	if(stat("/dev/sda", &s) == 0 || stat("/dev/sdb", &s) == 0){
		/* A physical drive is connected, check if it is mounted on /mnt */
		if((mounts = setmntent("/proc/mounts", "r")) != NULL){
			while((mount_info = getmntent(mounts)) != NULL) {
				if((mount_info->mnt_dir != NULL) && (strcmp(mount_info->mnt_dir, "/mnt") == 0)){
					is_mounted = true;
					break;
				}
			}

			/* Close the file */
			endmntent(mounts);
		}

		return is_mounted ? USB_DISK_MOUNTED : USB_DISK_NOT_MOUNTED;
	} else {
		return USB_DISK_NOT_INSTALLED;
	}
}

/*
 * Returns the free storage space on the usb
 * storage device when it is mounted on '/mnt'
 */
int system_get_usb_storage_freespace()
{
	/* The mount point we want to check */
	struct statfs s;
	statfs("/mnt", &s);

	return (int)((s.f_bavail * s.f_frsize)/1048576);
}

/*
 * Returns the total storage space on the usb
 * storage device when it is mounted on '/mnt'
 */
int system_get_usb_storage_totalspace()
{
	/* The mount point we want to check */
	struct statfs s;
	statfs("/mnt", &s);

	/* Return total disk space */
	return (int)((s.f_blocks * s.f_frsize)/1048576);
}

/*
 * Get the current system load in linux style
 */
char* system_get_system_load()
{
	int fd;				/* File descriptor*/
	char *buf = (char*) calloc(15, sizeof(char));

	/* Try to open device state file */
	fd = open("/proc/loadavg", O_RDONLY);
	if(fd < 0) {
		/* The file could not be opened */
		return NULL;
	}

	/* Read the system load file */
	if(read(fd, buf, 14) < 0) {
		return NULL;
	}

	/* Close the file */
	if(close(fd) < 0) {
		return NULL;
	}

	/* Return the load information */
	return buf;
}

/*
 * Get the free space in KiB of system RAM
 */
int system_get_ram_freespace()
{
	/* Get system information */
		struct sysinfo info;
		sysinfo(&info);

		return (int) info.freeram/1024;
}

/*
 * Get the total size in KiB of system RAM
 */
int system_get_ram_totalspace()
{
	/* Get system information */
	struct sysinfo info;
	sysinfo(&info);

	return (int) info.totalram/1024;
}
