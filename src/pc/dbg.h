#pragma once

#include <util/os.h>

#include <stdio.h>

extern char *prog_name;

void print_version(void);
NORETURN void print_usage(void);
NORETURN void print_usage_error(PRINTF_FORMAT const char *format, ...) PRINTF_FORMAT_ATTR(1, 2);
NORETURN void print_error(PRINTF_FORMAT const char *format, ...) PRINTF_FORMAT_ATTR(1, 2);

typedef enum debug_type_e
{
    debug_cpu,
    debug_int,
    debug_port,
    debug_dos,
    debug_video,
    debug_verbose, 
    debug_MAX
} debug_type;

void debug_init(const char *name);
void debug(debug_type, PRINTF_FORMAT const char *format, ...) PRINTF_FORMAT_ATTR(2, 3);
int debug_active(debug_type);
