#pragma once

#include <stdint.h>

/* 
    IBM VGA (1987) emulation
    Incl. Mode 4,5,6,7,8,9,10,11,12,13h...X,Y...Blah blah!
*/

// Enumerate VGA registers
// We don't care about mono
typedef enum vga_ports_e
{   
    // Write port for data, read/write for address(based on flipflop)
    VGA_ATTRIBUTE = 0x03C0,
    VGA_ATTRIBUTE_DATA_WRITE = 0x03C1,
    VGA_INPUT_0 = 0x03C1, // triggered by register (otherwise VGA_ATTRIBUTE_DATA_WRITE)
    VGA_MISC_WRITE = 0x03C2,
    VGA_ENABLE = 0x03C3,
    VGA_SEQ_ADDRESS = 0x03C4,
    VGA_SEQ_DATA = 0x03C5,
    VGA_PALETTE_MASK = 0x03C6,
    VGA_PALETTE_READ = 0x03C7,
    VGA_PALETTE_WRITE_ADDR = 0x03C8,
    VGA_PALETTE_DATA = 0x03C9,
    VGA_FEATURE_CONTROL = 0x03CA,
    VGA_INPUT_1_COLOR = 0x03CA,
    VGA_MISC_READ = 0x03CC,
    VGA_GRAPHICS_ADDRESS = 0x03CE,
    VGA_GRAPHICS_DATA = 0x03CF,
    VGA_CRTC_ADDRESS_COLOR = 0x03D4,
    VGA_CRTC_DATA_COLOR = 0x03D5,
} vga_ports;

#define VGA_NUM_CRTC        0x18
#define VGA_NUM_SEQ         4

typedef struct vga_s
{
    uint8_t attr;
    uint8_t input0;
    uint8_t input1;
    uint8_t crtc_addr;
    uint8_t crtc[VGA_NUM_CRTC];
    uint8_t seq_addr;
    uint8_t seq[VGA_NUM_SEQ];
    uint8_t gr_addr;
    uint8_t palette_read_addr;
    uint8_t palette_write_addr;
    uint8_t feature_control;
    bool flipflop;
} vga_t; 