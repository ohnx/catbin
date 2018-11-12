#include "main.h"

struct cb_settings cb_settings;



void cb_main_onsigint(uv_signal_t *handle, int signum) {
    if (signum == SIGINT)
        if (uv_loop_close(handle->loop) == UV_EBUSY)
            /* close all remaining connections */
            uv_walk(handle->loop, (uv_walk_cb)uv_close, free);
}


int main(int argc, char **argv) {
    int r, opt;
    uv_signal_t *sigint;

    /* initialize logger */
    cb_logger_init();

    /* default config options */
    cb_settings.port = 7777;
    cb_settings.slug_len_min = 4;
    cb_settings.host = "0.0.0.0";
    cb_settings.url = NULL;
    cb_settings.url_len = 0;

    while ((opt = getopt(argc, argv, "p:s:b:d:h")) != -1) {
        switch (opt) {
        case 'p':
            /* port */
            cb_settings.port = atoi(optarg);
            if (cb_settings.port < 0 || cb_settings.port > 65535) {
                cb_logger.log(FATL, "Invalid port specified\n");
                return -1;
            }
            break;
        case 's':
            /* minimum slug length */
            cb_settings.slug_len_min = atoi(optarg);
            break;
        case 'b':
            /* bindhost */
            cb_settings.host = optarg;
            break;
        case 'd':
            /* domain/url */
            cb_settings.url = optarg;
            cb_settings.url_len = strlen(cb_settings.url);
            if (cb_settings.url[cb_settings.url_len-1] != '/')
                cb_logger.log(WARN, "Given domain does not end with a /; this may cause domains to appear incorrectly.\n");
            break;
        case 'h':
        default:
            fprintf(stderr, "Usage:\n");
            fprintf(stderr, "\t%s [-p port (defaults to 7777)] [-s minimum slug length (defaults to 4)]\n\t\t", argv[0]);
            fprintf(stderr, "[-b bindhost (defaults to 0.0.0.0)] [-d domain]\n");
            return -1;
        }
    }

    if (cb_settings.url == NULL) {
        cb_logger.log(FATL, "Domain required; please run `%s -h` for help\n", argv[0]);
        return -1;
    }

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