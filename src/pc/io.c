// I/O Ports - X86 I/O Engine

#include <pc/dbg.h>
#include <emu.h>
#include <pc/keyb.h>
#include <pc/timer.h>
#include <pc/video.h>

uint8_t read_port(unsigned port)
{
    if(port == 0x3DA) // CGA status register
    {
        static int retrace = 0;
        retrace++;
        return (retrace >> 1) & 0x09;
    }
    else if(port == 0x3D4 || port == 0x3D5)
        return video_crtc_read(port);
    else if(port >= 0x40 && port <= 0x43)
        return port_timer_read(port);
    else if(port >= 0x60 && port <= 0x65)
        return keyb_read_port(port);
    debug(debug_port, "port read %04x\n", port);
    return 0xFF;
}

void write_port(unsigned port, uint8_t value)
{
    if(port >= 0x40 && port <= 0x43)
        port_timer_write(port, value);
    else if(port == 0x03D4 || port == 0x03D5)
        video_crtc_write(port, value);
    else if(port >= 0x60 && port <= 0x65)
        keyb_write_port(port, value);
    else
        debug(debug_port, "port write %04x <- %02x\n", port, value);
}