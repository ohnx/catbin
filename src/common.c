#include "common.h"

struct cb_settings cb_settings;
struct cb_logger cb_logger;
uv_loop_t *cb_loop = NULL;
uv_tcp_t *cb_server = NULL;
