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

    /* default config options */
    cb_settings.slug_len_min = 4;
    cb_settings.host = "0.0.0.0";
    cb_settings.port = 8791;
    cb_settings.url = "http://paste.example.com/";
    cb_settings.url_len = strlen(cb_settings.url);

#if 0
    while ((opt = getopt(argc, argv, "s:h:p:d:")) != -1) {
        switch (opt) {
        case 'p':
            /* port */
            cb_settings.port = atoi(optarg);
            if (cb_settings.port < 0 || cb_settings.port > 65535) {
                cb_logger.log(FATL, "Invalid port specified\n");
                return 53;
            }
            break;
        case 's':
            /* minimun slug length */
            if (server_sf == MAILBOX)
                server_sf = BOTH;
            else
                server_sf = STDOUT;
            break;
        case 'm':
            /* maildir */
            if (chdir(optarg)) {
                /* failed to cd to directory */
                print_usage(argv[0], "invalid output directory for mail");
                return 63;
            }
            if (server_sf == STDOUT)
                server_sf = BOTH;
            else
                server_sf = MAILBOX;
            break;
        case 'k':
            if (ssl_key) {
                print_usage(argv[0], "only 1 private key allowed");
                return 17;
            }
            ssl_key = optarg;
            enable_ssl++;
            break;
        case 'c':
            if (ssl_cert) {
                print_usage(argv[0], "only 1 certificate allowed");
                return 19;
            }
            ssl_cert = optarg;
            enable_ssl++;
            break;
        case 'u':
            break;
        default:
            /* unknown flag */
            cb_logger.log(FATL, "Unknown flag `%c`\n", opt);
            return -1;
        }
    }
#endif

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