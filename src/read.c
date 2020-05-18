#include "read.h"

void cb_read_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    /* Allocate suggested amount */
    buf->base = (char*)malloc(suggested_size);
    buf->len = suggested_size;
}

#if 0
int strncasecmp(const char *a, const char *b, ssize_t n) {
    /* complexity-saving measure: expects a to be all uppercase */
    while (*a && *b && n--) {
        printf("%c <-> %c; n = %d\n", *a, *b, n);
        if (*b >= 'a') {
            if (*(a++) != *(b++) - 32) return -1;
        } else {
            if (*(a++) != *(b++)) return -1;
        }
    }

    if (n != 0) return -1;

    return 0;
}
#endif

void cb_read_close(uv_timer_t* handle) {
    uv_stream_t *client = (uv_stream_t *)handle->data;
    uv_timer_stop(handle);
    cb_write_stop(client->data);
    uv_close((uv_handle_t *)client, (uv_close_cb)free);
    uv_close((uv_handle_t *)handle, (uv_close_cb)free);
}

void cb_read_ondata(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    struct rw_ifdata *data = (struct rw_ifdata *)client->data;
    uv_timer_t *close_timeout;
    ssize_t offset = 0, q = 0;

    if (nread < 0) {
        if (nread != UV_EOF) {
            cb_logger.log(WARN, "Read error %s\n", uv_err_name(nread));
        } else {
            cb_logger.log(INFO, "Client disconnected!\n");
        }

        if (data->flags != FLAG_CONN_CLOSING) {
            /* close immediately */
            cb_write_stop(client->data);
            uv_close((uv_handle_t *)client, (uv_close_cb)free);
            free(buf->base);
        } /* otherwise we are already going to close it */
    } else if (nread > 0) {
        cb_logger.log(DEBG, "Read %d bytes from client!\n", nread);
    rset:
        switch (data->flags) {
        case FLAG_INITIAL_READ:
            /* check if this data starts with "PUT /" */
            if (!strncmp("PUT /", buf->base, sizeof("PUT /") - 1)) {
                /* this is a HTTP PUT request */
                cb_logger.log(DEBG, "HTTP PUT request for fd #%d!\n", data->fd);
                data->flags = FLAG_HTTP_PUT_HEADERS;
            } else if (!strncmp("OPTIONS /", buf->base, sizeof("OPTIONS /") - 1)) {
                /* this is a HTTP OPTIONS request */
                cb_logger.log(DEBG, "HTTP OPTIONS request for fd #%d!\n", data->fd);
                data->flags = FLAG_HTTP_OPTIONS_HEADERS;
                data->d_expected = 0;
            } else if (*(buf->base) == 0x16) {
                /* TLS request */
                cb_logger.log(DEBG, "TLS request for fd #%d!\n", data->fd);
                data->flags = FLAG_BINARY_DATA;
            } else {
                /* regular request */
                cb_logger.log(DEBG, "Binary request for fd #%d! %x %x\n", data->fd, *(buf->base), *(buf->base+1));
                data->flags = FLAG_BINARY_DATA;
            }
            /* write the hello message */
            cb_write_hello(data);
            goto rset;
        case FLAG_HTTP_PUT_HEADERS:
            /* consume the headers and stuff */
            cb_logger.log(DEBG, "Consuming headers for fd #%d!\n", data->fd);
        consume:
            /* skip until we encounter a carriage return */
            while (offset <= nread && *(buf->base + offset) && *(buf->base + offset) != '\r') {
                if ((*(buf->base + offset) == 'C' || *(buf->base + offset) == 'c')
                    && offset > 0 && *(buf->base + offset - 1) == '\n') {
                    /* Maybe this is a content-length header?? :o */
                    if (!strncasecmp("CONTENT-LENGTH: ", buf->base + offset, sizeof("CONTENT-LENGTH: ") - 1)) {
                        /* this is indeed a content-length header, try to parse a long out of it! */
                        q = offset + sizeof("CONTENT-LENGTH: ") - 1;
                        while (q <= nread && *(buf->base + q) && *(buf->base + q) != '\r') q++;

                        /* reached the end of the line. try seeing if we can grab the number */
                        if (*(buf->base + q) == '\r') {
                            *(buf->base + q) = '\0';
                            data->d_expected = atol(buf->base + offset + sizeof("CONTENT-LENGTH: ") - 1);
                            if (data->d_expected < 0 || (cb_settings.max_size && data->d_expected > cb_settings.max_size))
                                data->d_expected = cb_settings.max_size;
                            *(buf->base + q) = '\r';
                        }
                        offset = q-1; /* might as well as save some reading time :) */
                    }
                }
                offset++;
            }
            if (offset > nread || *(buf->base + offset) == '\0') {
                /* end of string, we have consumed the entire buffer without reaching a double CRLF */
                free(buf->base);
                break;
            } else if (*(buf->base + offset) == '\r' && *(buf->base + offset + 1) == '\n') {
                /* CRLF reached */
                if (!(*(buf->base + offset + 2) == '\r' && *(buf->base + offset + 3) == '\n')) {
                    /* just 1 CRLF, so keep trying to read more headers */
                    offset++;
                    goto consume;
                }
                /* 2 CRLF's in a row = PUT data starting */
                cb_logger.log(DEBG, "Data starting for fd #%d!\n", data->fd);
                offset += 4; /* skip past the \r\n\r\n */
            }
            data->flags = FLAG_HTTP_PUT_DATA;
        case FLAG_BINARY_DATA: /* regular data */
        case FLAG_HTTP_PUT_DATA: /* HTTP PUT data */
            /* check we have enough space */
            if (data->d_expected && data->d_written + (nread - offset) >= data->d_expected) {
                if (data->d_expected == cb_settings.max_size)
                    cb_logger.log(INFO, "Data limit of %d reached for fd #%d!\n", data->d_expected, data->fd);
                else
                    cb_logger.log(INFO, "Client done sending file!\n");

                cb_write(data, data->d_expected - data->d_written, offset, buf);
                /* cb_write will free the buffer */

                close_timeout = malloc(sizeof(uv_timer_t));
                if (!close_timeout) {
                    /* close immediately */
                    cb_write_stop(client->data);
                    uv_close((uv_handle_t *)client, (uv_close_cb)free);
                } else {
                    /* close with timeout */
                    data->flags = FLAG_CONN_CLOSING;
                    close_timeout->data = client;
                    uv_timer_init(cb_loop, close_timeout);
                    uv_timer_start(close_timeout, cb_read_close, 500, 0);
                }
                break;
            }

            /* not a problem */
            cb_write(data, nread - offset, offset, buf);
            data->d_written += nread;
            break;
        }
    } else {
        free(buf->base);
    }
}

void cb_read_onconn(uv_stream_t *server, int status) {
    struct rw_ifdata *data;

    if (status < 0) {
        cb_logger.log(WARN, "Error with new connection: %s\n", uv_strerror(status));
        return;
    }

    /* allocate space for read/write callback context stuff + client */
    data = malloc(sizeof(struct rw_ifdata));
    if (!data) {
        cb_logger.log(WARN, "OOM Error when accepting new connection\n");
        return;
    }
    data->client.data = data;
    data->server = server;
    data->flags = FLAG_INITIAL_READ;
    data->d_written = 0;
    data->d_expected = cb_settings.max_size;

    cb_logger.log(INFO, "Initiating new connection!\n");

    /* initialize stuff */
    uv_tcp_init(cb_loop, &data->client);

    /* open the file and stuff */
    cb_write_start(data);
}

int cb_read_init() {
    struct sockaddr_in addr;

    cb_logger.log(INFO, "cb_read_init() binding %s:%d\n", cb_settings.host, cb_settings.port);

    /* Set loop */
    if (!cb_loop)
        cb_loop = uv_default_loop();

    /* Allocate memory for server */
    cb_server = malloc(sizeof(uv_tcp_t));
    if (!cb_server) return UV_ENOMEM;

    /* Initialize server */
    uv_tcp_init(cb_loop, cb_server);
    uv_ip4_addr(cb_settings.host, cb_settings.port, &addr);
    uv_tcp_bind(cb_server, (const struct sockaddr *)&addr, 0);

    return uv_listen((uv_stream_t *)cb_server, 128, &cb_read_onconn);
}
