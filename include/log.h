#ifndef __LOG_H_INC
#define __LOG_H_INC

#include <stdio.h>
#include <stdarg.h>

#include "common.h"

enum cb_logger_level {
    DEBG = 0,
    INFO = 1,
    WARN = 2,
    EROR = 4,
    FATL = 8
};

struct cb_logger {
    enum cb_logger_level level;
    void (*log)(enum cb_logger_level, const char *, ...);
};

int cb_logger_init();
#endif /* __LOG_H_INC */
