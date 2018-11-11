#include "read.h"

void cb_read_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    /* Allocate suggested amount */
    buf->base = (char*)malloc(suggested_size);
    buf->len = suggested_size;
}

void cb_read_ondata(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread < 0) {
        if (nread != UV_EOF) {
            cb_logger.log(WARN, "Read error %s\n", uv_err_name(nread));
        } else {
            cb_logger.log(DEBG, "Client disconnected and file written!\n");
        }
        cb_write_stop(client->data);
        uv_close((uv_handle_t *)client, (uv_close_cb)free);
        free(buf->base);
    } else if (nread > 0) {
        cb_logger.log(DEBG, "Read %d bytes from client!\n", nread);
        cb_write((struct rw_ifdata *)client->data, nread, buf);
        /* cb_write will free the buffer */
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

    cb_logger.log(DEBG, "Initiating new connection!\n");
    /* open the file and stuff */
    cb_write_start(data);

    /* initialize stuff */
    uv_tcp_init(cb_loop, &data->client);
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
