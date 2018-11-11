#include "log.h"

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

static const char *_cb_logger_level_str[] = {
    "["MAGENTA"DEBG"RESET"]",
    "["BLUE"INFO"RESET"]",
    "["YELLOW"WARN"RESET"]",
    "["RED"EROR"RESET"]",
    "["RED"FATL"RESET"]"
};

void cb_logger_log(enum cb_logger_level level, const char *fmt, ...) {
    va_list x;
    const char *prefix;
    time_t t;
    struct tm tm;

    /* skip output if lower level */
    if (level < cb_logger.level) return;

    /* get the right string */
    switch (level) {
        case DEBG: prefix = _cb_logger_level_str[0]; break;
        case INFO: prefix = _cb_logger_level_str[1]; break;
        case WARN: prefix = _cb_logger_level_str[2]; break;
        case EROR: prefix = _cb_logger_level_str[3]; break;
        case FATL: prefix = _cb_logger_level_str[4]; break;
        default: prefix = _cb_logger_level_str[0]; break;
    }

    /* get current time */
    t = time(NULL);
    tm = *localtime(&t);

    /* output level + time */
    fputs(prefix, stderr);
    fprintf(stderr, "[%04d-%02d-%02d %02d:%02d:%02d] ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    /* output actual text */
    va_start(x, fmt);
    vfprintf(stderr, fmt, x);
    va_end(x);
}

int cb_logger_init() {
#ifdef __DEBUG
    cb_logger.level = DEBG;
#else
    cb_logger.level = INFO;
#endif
    cb_logger.log = &cb_logger_log;

    return 0;
}
