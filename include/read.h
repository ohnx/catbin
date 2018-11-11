#ifndef __READ_H_INC
#define __READ_H_INC

#include <stdlib.h>
#include <uv.h>
#include "common.h"
#include "write.h"

void cb_read_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
int cb_read_init();
void cb_read_onconn(uv_stream_t *server, int status);
void cb_read_ondata(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);

#endif /* __READ_H_INC */
