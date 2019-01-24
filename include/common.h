#ifndef __COMMON_H_INC
#define __COMMON_H_INC

#include <stdint.h>
#include <uv.h>
#include "log.h"

struct cb_settings {
    /**
     * @brief URL used in links
     * e.g. "https://example.com/" will output "https://example.com/xxxxxx"
     */
    char *url;
    uint16_t url_len;

    /**
     * @brief Host to bind to
     */
    const char *host;

    /**
     * @brief Port to listen on
     * e.g. 9999
     */
    uint16_t port;

    /**
     * @brief Minimum length of a file ID
     */
    uint8_t slug_len_min;

    /**
     * @brief Maximum permitted file size (in bytes)
     */
    ssize_t max_size;
};

struct rw_ifdata {
    /* IMPORTANT HACK to keep client first so that when uv goes to free stuff
     we aren't trying to free a pointer inside of an allocated space */
    uv_tcp_t client;
    uv_file fd;
    uv_stream_t *server;
    char *slug;
    unsigned char flags;
    ssize_t d_written;
    ssize_t d_expected;
};

/* global variables */
extern struct cb_settings cb_settings;
extern struct cb_logger cb_logger;
extern uv_loop_t *cb_loop;
extern uv_tcp_t *cb_server;

#endif /* __COMMON_H_INC */
