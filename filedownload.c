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
 * File:   filedownload.c
 * Created on January 25, 2015, 8:10 AM
 */

#include <curl/curl.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "filedownload.h"
#include "logger.h"

/**
 * Arguments for the download thread
 */
struct _download_file_args {
    char* url; 
    char* path; 
    char*name;
    struct download_info* d_info;
    void *(*callback) (struct download_info *);
};

/**
 * Free the contents of the the downloads arguments.
 * @param args the arguments to free.
 */
static void _free_download_args(struct _download_file_args *args)
{
    free(args->url);
    free(args->path);
    free(args->name);
}

static size_t _download_write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

/**
 * The entry point of the download file thread. Arguments is a struct that will be
 * freed by this thread. 
 * @param arguments the arguments for this entry point. 
 */
static void* _download_file(void *arguments)
{
    struct _download_file_args *args = (struct _download_file_args*) arguments;
    struct download_info *d_info = args->d_info;
    d_info->status = DOWNLOAD_BUSY;
    log_message(LOG_INFO, "Started downloading file: '%s'\r\n", args->url);
    
    CURL *curl;
    FILE *fd;
    CURLcode res;
    
    curl = curl_easy_init();
    if(curl) {
        fd = fopen(args->path, "wb");
        curl_easy_setopt(curl, CURLOPT_URL, args->url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _download_write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fd);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            log_message(LOG_ERROR, "Curl download failed: %s\r\n", curl_easy_strerror(res));
            d_info->status = DOWNLOAD_FAIL;
        }
        curl_easy_cleanup(curl);
        fclose(fd);
    }
    d_info->status = DOWNLOAD_SUCCESS;
    
    /* Call callback */
    args->callback(d_info);
    
    /* Free arguments */
    _free_download_args(arguments);
    free(arguments);
    pthread_exit(NULL);
}

/**
 * Fire an async file download, strings will be freed when the download
 * is complete. The strings will be copied, so you can free memory. 
 * @param url the url to download.
 * @param path the directory to save the downloaded file in.
 * @param name the name to save the file under.
 * @param d_info structure in which download information will be written.
 * @param callback function to call when download is finished
 * @return true when the download started.
 */
bool filedownload_async_download(char* url, char* path, char*name, struct download_info* d_info, void *(*callback) (struct download_info *))
{
    pthread_t thread;
    struct _download_file_args *args = (struct _download_file_args*) calloc(1, sizeof(struct _download_file_args));
    
    args->url = (char*) malloc((strlen(url) + 1)*sizeof(char));
    strcpy(args->url, url);
    
    args->path = (char*) malloc((strlen(path) + 1)*sizeof(char));;
    strcpy(args->path, path);
    
    args->name = (char*) malloc((strlen(name) + 1)*sizeof(char));;
    strcpy(args->name, name);
    
    args->d_info = d_info;
    args->callback = callback;
    
    /* Start the download */
    if(pthread_create(&thread, NULL, _download_file, (void*) args) != 0) {
        log_message(LOG_ERROR, "Could not start download thread\r\n");
        return false;
    }
    pthread_detach(thread);
    
    return true;
}
