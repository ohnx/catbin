#ifndef __WRITE_H_INC
#define __WRITE_H_INC

#include <string.h>
#include <stdlib.h>
#include <uv.h>
#include "common.h"

int cb_write_allocslug(char **slug);
int cb_write_init();
void cb_write_onopen(uv_fs_t *req);
void cb_write_onwrite(uv_fs_t *req);
int cb_write_start(void **fdptr);
void cb_write(uv_file *fd, ssize_t nread, const uv_buf_t *buf);

#endif /* __WRITE_H_INC */
