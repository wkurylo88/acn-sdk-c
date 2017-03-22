/* Copyright (c) 2017 Arrow Electronics, Inc.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Apache License 2.0
 * which accompanies this distribution, and is available at
 * http://apache.org/licenses/LICENSE-2.0
 * Contributors: Arrow Electronics, Inc.
 */

#define MODULE_NAME "HTTP_Request"

#include "http/request.h"
#include <debug.h>
#include <config.h>
#if defined(__USE_STD__)
# include <string.h>
# include <stdio.h>
# include <stdlib.h>
#endif
#include <arrow/mem.h>

static const char *meth_str[5] = {"GET", "POST", "PUT", "DELETE", "HEAD"};

int cmp_meth(const char *str) {
  int i;
  for(i=0; i<5; i++) {
    if ( strcmp(str, meth_str[i]) == 0 ) return i;
  }
  return -1;
}

char *get_meth(int i) {
    if ( i>=0 && i<5 ) {
        return (char*)meth_str[i];
    }
    return NULL;
}

void http_request_init(http_request_t *req, int meth, const char *url) {
    req->meth = (uint8_t*)meth_str[meth];
    char *sch_end = strstr((char*)url, "://");
    if ( !sch_end ) { req->is_corrupt = 1; return; }
    req->scheme = (uint8_t*)malloc((size_t)(sch_end - url) + 1);
    strncpy((char*)req->scheme, url, (size_t)(sch_end - url));
    req->scheme[sch_end - url] = '\0';
    char *host_start = sch_end + 3; //sch_end + strlen("://");
    char *host_end = strstr(host_start, ":");
    if ( !host_end ) { req->is_corrupt = 1; return; }
    strncpy((char*)req->host, host_start, (size_t)(host_end - host_start));
    req->host[host_end - host_start] = '\0';
    int res = sscanf(host_end+1, "%hu", &req->port);
    if ( res!=1 ) { req->is_corrupt = 1; return; }
    char *uri_start = strstr(host_end+1, "/");
    strncpy((char*)req->uri, uri_start, strlen(uri_start));
    req->uri[strlen(uri_start)] = '\0';
    if (strcmp((char*)req->scheme, "https")==0) req->is_cipher = 1;
    else req->is_cipher = 0;
    DBG("meth: %s", req->meth);
    DBG("scheme: %s", req->scheme);
    DBG("host: %s", req->host);
    DBG("port: %d", req->port);
    DBG("uri: %s", req->uri);
    DBG("res: %d", res);

    req->header = NULL;
    req->query = NULL;

    req->is_chunked = 0;

    req->payload.size = 0;
    req->payload.buf = NULL;

    req->content_type.key = NULL;
    req->content_type.value = NULL;
    req->content_type.next = NULL;
}

void http_request_close(http_request_t *req) {
    if (req->scheme) free(req->scheme);
    if (req->payload.buf) free(req->payload.buf);
    req->payload.size = 0;
    http_header_t *head = req->header;
    http_header_t *head_next = NULL;
    do {
        if (head) {
            head_next = head->next;
            if (head->key) free(head->key);
            if (head->value) free(head->value);
            free(head);
        }
        head = head_next;
    } while(head);
    if (req->content_type.value) free(req->content_type.value);
}

void http_response_free(http_response_t *res) {
    if (res->payload.buf) free(res->payload.buf);
    res->payload.size = 0;
    http_header_t *head = res->header;
    http_header_t *head_next = NULL;
    do {
        if (head) {
            head_next = head->next;
            if (head->key) free(head->key);
            if (head->value) free(head->value);
            free(head);
        }
        head = head_next;
    } while(head);
    if (res->content_type.value) free(res->content_type.value);
}

void http_request_add_header(http_request_t *req, const char *key, const char *value) {
    http_header_t *head = req->header;
    while( head && head->next ) {
//        if ( head ) {
//            DBG("--:{%s}", head->value);
//        }
        head = head->next;
    }
    http_header_t *head_new = (http_header_t *)malloc(sizeof(http_header_t));
    head_new->key = (uint8_t*)malloc(strlen(key)+1);
    head_new->value = (uint8_t*)malloc(strlen(value)+1);
    strcpy((char*)head_new->key, key);
    strcpy((char*)head_new->value, value);
    head_new->next = NULL;
    if ( head ) head->next = head_new;
    else req->header = head_new;
}

void http_request_set_content_type(http_request_t *req, const char *value) {
    req->content_type.value = (uint8_t*)malloc(strlen(value)+1);
    strcpy((char*)req->content_type.value, value);
}

http_header_t *http_request_first_header(http_request_t *req) {
    return req->header;
}

http_header_t *http_request_next_header(http_request_t *req, http_header_t *head) {
    SSP_PARAMETER_NOT_USED(req);
    if ( ! head ) return NULL;
    return head->next;
}

static void set_payload(http_payload_t *pay, const char *buf, size_t size) {
    pay->size = size;
    if ( pay->buf ) {
        pay->buf = (uint8_t*)realloc(pay->buf, size+1);
    } else {
        pay->buf = (uint8_t*)malloc(size+1);
    }
    memcpy(pay->buf, buf, size);
    pay->buf[pay->size] = '\0';
}

void http_request_set_payload(http_request_t *req, char *payload) {
    set_payload(&req->payload, payload, strlen(payload));
}

void http_response_add_header(http_response_t *req, const char *key, const char *value) {
    http_header_t *head = req->header;
    while( head && head->next ){
        head = head->next;
    }
    http_header_t *head_new = (http_header_t *)malloc(sizeof(http_header_t));
    head_new->key = (uint8_t*)malloc(strlen(key)+1);
    head_new->value = (uint8_t*)malloc(strlen(value)+1);
    strcpy((char*)head_new->key, key);
    strcpy((char*)head_new->value, (char*)value);
    head_new->next = NULL;
    if ( head ) head->next = head_new;
    else req->header = head_new;
}

void http_response_set_content_type(http_response_t *res, const char *value) {
    res->content_type.value = (uint8_t*)malloc(strlen(value)+1);
    if ( !res->content_type.value ) {
        DBG("resp set content type malloc fail");
        return;
    }
    strcpy((char*)res->content_type.value, value);
}

void http_response_set_payload(http_response_t *res, char *payload, size_t size) {
    if ( !size ) size = strlen(payload);
    set_payload(&res->payload, payload, size);
}

void http_response_add_payload(http_response_t *res, char *payload, size_t size) {
    if ( !size ) size = strlen(payload);
    if ( !res->payload.buf ) {
        set_payload(&res->payload, payload, size);
        return;
    } else {
        res->payload.size += size;
        uint8_t *old_buf = res->payload.buf;
        res->payload.buf = (uint8_t*)malloc(res->payload.size+1);
        memcpy(res->payload.buf, old_buf, res->payload.size - size);
        free(old_buf);
        memcpy(res->payload.buf + res->payload.size - size, payload, size);
        res->payload.buf[res->payload.size] = '\0';
    }
}