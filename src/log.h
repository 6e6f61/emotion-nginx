#ifndef _LOG_H_
#define _LOG_H_
#include <stdio.h>
#include <debug.h>

#define multi_log(fmt, ...) do { \
        printf("emotion-nginx: " fmt, ##__VA_ARGS__); \
        scr_printf(fmt, ##__VA_ARGS__); \
    } while (0)
#endif