#include "write.h"

static const char *_cb_write_dic = "abcdefghijkmnoprstuvwxyz0123456789";
static uint8_t _cb_write_dic_len;

/**
 * Allocate a new slug
 */
int cb_write_allocslug(char **slug) {
    uint8_t i;
    FILE *f;
    uint8_t slug_len = cb_settings.slug_len_min;
    void *ptr;
    *slug = malloc(sizeof(char) * (slug_len + 1));

    if (!(*slug)) {
        cb_logger.log(WARN, "OOM error when trying to allocate memory for slug\n");
        return UV_ENOMEM;
    }

    /* initial allocation */
    for (i = 0; i < slug_len; i++) {
        (*slug)[i] = _cb_write_dic[rand() % _cb_write_dic_len];
    }
    (*slug)[i] = '\0';

    /* keep trying to read the file */
    while ((f = fopen(*slug, "r"))) {
        /* successful read = file exists */
        fclose(f);

        /* increase memory if necessary */
        if (i == slug_len) {
            /* sanity check */
            if (slug_len >= 128) {
                cb_logger.log(EROR, "Error trying to find unused slug\n");
                free(*slug);
                return UV_ENOMEM;
            }

            /* increase slug length allocation by 8 to save some realloc calls */
            slug_len += 8;

            /* realloc + catch errors */
            ptr = realloc(*slug, sizeof(char) * (slug_len + 1));
            if (!ptr) {
                cb_logger.log(WARN, "OOM error when trying to allocate memory for slug\n");
                free(*slug);
                return UV_ENOMEM;
            }
            *slug = (char *)ptr;
        }

        /* add 1 character to the slug */
        *slug[i++] = _cb_write_dic[rand() % _cb_write_dic_len];
        *slug[i] = '\0';
    }

    /* read failed */
    return 0;
}

void cb_write_stop(struct rw_ifdata *data) {
    /* allocate space for file handler */
    uv_fs_t *req = malloc(sizeof(uv_fs_t));

    if (!req) {
        cb_logger.log(WARN, "OOM error when trying to allocate memory for output file\n");
        return;
    }

    uv_fs_close(cb_loop, req, data->fd, (uv_fs_cb)free);
}

void cb_write_onwrite(uv_fs_t *req) {
    if (req->result < 0) {
        cb_logger.log(WARN, "Nonzero status from uv_write(): %s\n", uv_strerror(req->result));
    } else {
        cb_logger.log(DEBG, "Wrote %d bytes to file!\n", req->result);
    }

    free(req->data);
    uv_fs_req_cleanup(req);
    free(req);
}

void cb_write(struct rw_ifdata *data, ssize_t nread, const uv_buf_t *buf) {
    uv_fs_t *req;
    uv_buf_t wrbuf;

    req = malloc(sizeof(uv_fs_t));
    if (!req) {
        cb_logger.log(WARN, "OOM in server\n");
        free(buf->base);
        return;
    }

    wrbuf = uv_buf_init(buf->base, nread);
    req->data = buf->base;
    cb_logger.log(DEBG, "Writing %d bytes to file %d\n", nread, data->fd);
    uv_fs_write(cb_loop, req, data->fd, &wrbuf, 1, 0, cb_write_onwrite);
}

void cb_write_onopen_onwrite(uv_write_t* req, int status) {
    if (status < 0) {
        cb_logger.log(WARN, "Failed to communicate with client: %s\n", uv_strerror(status));
    }
    free(req);
}

void cb_write_onopen(uv_fs_t *req) {
    uv_write_t *wreq;
    struct rw_ifdata *data = (struct rw_ifdata *)req->data;
    uv_buf_t bufs[] = {
        {.base = (char *)cb_settings.url, .len = cb_settings.url_len},
        {.base = data->slug, .len = 0}
    };

    /* error occurred */
    if (req->result < 0) {
        cb_logger.log(WARN, "Failed to open output file: %s\n", uv_strerror(req->result));
        uv_fs_req_cleanup(req);
        free(req);
        free(data->slug);
        return;
    }

    /* store the resultant fd */
    data->fd = req->result;
    cb_logger.log(DEBG, "Successfully opened output file as fd #%d\n", data->fd);

    uv_fs_req_cleanup(req);
    free(req);

    /* write the network request */
    wreq = malloc(sizeof(uv_write_t));
    if (!req) {
        cb_logger.log(WARN, "OOM in server\n");
        free(data->slug);
        return;
    }

    /* start listening for incoming messages */
    if (!uv_accept(data->server, (uv_stream_t *)&data->client)) {
        uv_read_start((uv_stream_t *)&data->client, &cb_read_alloc, cb_read_ondata);
        bufs[1].len = strlen(data->slug);
        uv_write(wreq, (uv_stream_t *)&data->client, bufs, 2, cb_write_onopen_onwrite);
    } else {
        /* failed to accept() */
        uv_close((uv_handle_t *)&data->client, (uv_close_cb)free);
    }
    free(data->slug);
}

int cb_write_start(struct rw_ifdata *data) {
    int r;
    uv_fs_t *file;

    /* allocate slug */
    if ((r = cb_write_allocslug(&(data->slug)))) return r;

    /* allocate space for file handler */
    file = malloc(sizeof(uv_fs_t));
    if (!file) {
        cb_logger.log(WARN, "OOM error when trying to allocate memory for output file\n");
        free(data->slug);
        return UV_ENOMEM;
    }

    cb_logger.log(DEBG, "Opening file %s for writing!\n", data->slug);
    file->data = data;

    r = uv_fs_open(cb_loop, file, data->slug, O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR, &cb_write_onopen);
    if (r) {
        cb_logger.log(WARN, "Failed to open output file `%s`\n", data->slug);
        free(data->slug);
        free(file);
        return UV_ENOMEM;
    }
    return 0;
}


int cb_write_init() {
    srand(time(NULL));
    _cb_write_dic_len = strlen(_cb_write_dic);

    return 0;
}
