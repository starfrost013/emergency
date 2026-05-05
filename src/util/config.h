#pragma once

#include "emu.h"
#include "debug.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

/* Config system */

#define CONFIG_FILE_NAME        "emergency.cfg" // Config file name


typedef struct config_s
{
    debug_type debug_type;                      // Formerly ENV_DEBUG
    char debug_name[STRING_MAX_PATH];           // Formerly ENV_DBG_NAME    
    char progname[STRING_MAX_PATH];             // Formerly ENV_PROGNAME    
    char cwd[STRING_MAX_PATH];                  // Formerly ENV_CWD    
    char append[STRING_MAX_PATH];               // Formerly ENV_APPEND    
    uint8_t dos_major;                          // Formerly ENV_DOSVER
    uint8_t dos_minor;                          // Formerly ENV_DOSVER
    uint16_t codepage;                          // Formerly ENV_CODEPAGE
    size_t cpu_speed;                           // Formerly ENV_CPU_SPEED
    int32_t rows;                               // Formerly ENV_ROWS
    uint8_t default_drive;                      // Formerly ENV_DEF_DRIVE
    bool lowmem;                                // Formerly ENV_LOWMEM
    
    
    FILE* stream;                               // not technically a stream but i like the word stream 
    char current_section[STRING_MAX_PATH];      // optimisation: don't search if we alreayd have a path selected
    // don't store DRIVE_* directly
} config_t;

extern config_t config;

void config_init();
char* config_get_string(const char* section, const char* name);
uint8_t config_get_uint8(const char* section, const char* name);
int8_t config_get_int8(const char* section, const char* name);
uint16_t config_get_uint16(const char* section, const char* name);
int16_t config_get_int16(const char* section, const char* name);
uint32_t config_get_uint32(const char* section, const char* name);
int32_t config_get_int32(const char* section, const char* name);
void config_shutdown();

