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
 * File:   filedownload.h
 * Created on January 25, 2015, 8:10 AM
 */

#ifndef FILEDOWNLOAD_H
#define	FILEDOWNLOAD_H

#include <stdbool.h>
#include <stddef.h>


#define DOWNLOAD_SUCCESS    0
#define DOWNLOAD_BUSY       1
#define DOWNLOAD_FAIL       2

struct download_info {
    int status;
    int progress;
    size_t total_size;
    size_t downloaded;
};

/**
 * Fire an async file download, strings will be freed when the download
 * is complete. 
 * @param url the url to download.
 * @param path the directory to save the downloaded file in.
 * @param name the name to save the file under.
 * @param d_info structure in which download information will be written.
 * @param callback function to call when download is finished
 * @return true when the download started.
 */
bool filedownload_async_download(char* url, char* path, char*name, struct download_info* d_info, void *(*callback) (struct download_info *));

#endif

