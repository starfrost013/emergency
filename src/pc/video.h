#pragma once

#include <stdint.h>

void video_text_intr10(void);
// Redraws terminal screen
void video_check_screen(void);
// Returns 1 if video emulation is active.
int video_active(void);
// Writes a character to the video screen
void video_putch(char ch);
// Get current column in current page
int video_get_col(void);
// CRTC port read/write
uint8_t video_crtc_read(int port);
void video_crtc_write(int port, uint8_t value);
// Initializes emulated video memory and tables
void video_init_mem(void);

typedef struct video_gpu_s
{
    void (*gpu_init)(void);
    void (*gpu_shutdown)(void);
    void (*gpu_init_mem)(void);
    void (*gpu_reg_read8)(uint8_t addr);
    void (*gpu_reg_write8)(uint8_t addr, uint8_t value);
    void (*gpu_int10)();
} video_gpu_t; 

extern video_gpu_t video_gpus[];