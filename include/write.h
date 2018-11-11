#ifndef __WRITE_H_INC
#define __WRITE_H_INC

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>
#include "common.h"
#include "read.h"

int cb_write_allocslug(char **slug);
int cb_write_init();
void cb_write_onopen(uv_fs_t *req);
void cb_write_onwrite(uv_fs_t *req);
int cb_write_start(struct rw_ifdata *data);
void cb_write(struct rw_ifdata *data, ssize_t nread, const uv_buf_t *buf);
void cb_write_stop(struct rw_ifdata *data);

#endif /* __WRITE_H_INC */
