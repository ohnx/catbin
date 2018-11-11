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
    *slug = (char *)malloc(sizeof(char) * (slug_len + 1));

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
    close(data->fd);
}

void cb_write_onwrite(uv_fs_t *req) {
    if (req->result < 0) {
        cb_logger.log(WARN, "Nonzero status from uv_write(): %s\n", uv_strerror(req->result));
    }
    cb_logger.log(DEBG, "Wrote %d bytes to file!\n", req->result);
    free(req->data);
    uv_fs_req_cleanup(req);
    free(req);
}

void cb_write(struct rw_ifdata *data, ssize_t nread, const uv_buf_t *buf) {
    uv_fs_t *req = (uv_fs_t *)malloc(sizeof(uv_fs_t));
    uv_buf_t wrbuf = uv_buf_init(buf->base, nread);
    req->data = buf->base;
    cb_logger.log(DEBG, "Writing %d bytes to file %d: ```\n%.*s```\n", nread, data->fd, nread, buf->base);
    uv_fs_write(cb_loop, req, data->fd, &wrbuf, 1, 0, cb_write_onwrite);
}

void cb_write_onopen(uv_fs_t *req) {
    struct rw_ifdata *data = (struct rw_ifdata *)req->data;

    /* error occurred */
    if (req->result < 0) {
        cb_logger.log(WARN, "Failed to open output file: %s\n", uv_strerror(req->result));
        uv_fs_req_cleanup(req);
        return;
    }

    /* store the resultant fd */
    data->fd = req->result;
    cb_logger.log(DEBG, "Successfully opened output file as fd #%d\n", data->fd);

    uv_fs_req_cleanup(req);
    free(req);

    /* start listening for incoming messages */
    if (!uv_accept(data->server, (uv_stream_t *)&data->client)) {
        uv_read_start((uv_stream_t *)&data->client, &cb_read_alloc, *cb_read_ondata);
    } else {
        /* failed to accept() */
        uv_close((uv_handle_t *)&data->client, (uv_close_cb)free);
    }
}

int cb_write_start(struct rw_ifdata *data) {
    int r;
    char *slug;
    uv_fs_t *file;

    /* allocate slug */
    if ((r = cb_write_allocslug(&slug))) return r;

    /* allocate space for file handler */
    file = (uv_fs_t *)malloc(sizeof(uv_fs_t));
    if (!file) {
        cb_logger.log(WARN, "OOM error when trying to allocate memory for output file\n");
        free(slug);
        return UV_ENOMEM;
    }

    cb_logger.log(DEBG, "Opening file %s for writing!\n", slug);
    file->data = data;

    r = uv_fs_open(cb_loop, file, slug, O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR, &cb_write_onopen);
    if (r) {
        cb_logger.log(WARN, "Failed to open output file `%s`\n", slug);
        free(slug);
        free(file);
        return UV_ENOMEM;
    }
    free(slug);
    return 0;
}


int cb_write_init() {
    srand(time(NULL));
    _cb_write_dic_len = strlen(_cb_write_dic);

    return 0;
}
