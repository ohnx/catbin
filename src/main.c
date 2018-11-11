#include "main.h"

struct cb_settings cb_settings;



void cb_main_onsigint(uv_signal_t *handle, int signum) {
    if (signum == SIGINT)
        if (uv_loop_close(handle->loop) == UV_EBUSY)
            /* close all remaining connections */
            uv_walk(handle->loop, (uv_walk_cb)uv_close, free);
}


int main() {
    int r;
    uv_signal_t *sigint;

    /* initialize logger */
    cb_logger_init();

    cb_settings.slug_len_min = 4;
    cb_settings.host = "0.0.0.0";
    cb_settings.port = 8791;

    sigint = malloc(sizeof(uv_signal_t));
    if (!sigint) {
        cb_logger.log(FATL, "Error initializing program: OOM\n");
        return -1;
    }

    /* Set loop */
    if (!cb_loop)
        cb_loop = uv_default_loop();

    /* initialize network read functions */
    if ((r = cb_read_init())) {
        cb_logger.log(FATL, "Error initializing: %s\n", uv_strerror(r));
        return -1;
    } else if ((r = cb_write_init())) {
        cb_logger.log(FATL, "Error initializing program!\n");
        return -1;
    }

    /* catch SIGINT */
    uv_signal_init(cb_loop, sigint);
    uv_signal_start(sigint, cb_main_onsigint, SIGINT);

    /* loop */
    uv_run(cb_loop, UV_RUN_DEFAULT);
    cb_logger.log(FATL, "Received signal instructing to close\n");
    return uv_loop_close(cb_loop);
}