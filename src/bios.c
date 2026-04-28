/* bios.c: System BIOS */

#include "dbg.h"
#include "emu.h"
#include "keyb.h"

// BIOS - GET EQUIPMENT FLAG
void intr11(void)
{
    cpuSetAX(0x0021);
}

// BIOS - GET MEMORY
void intr12(void)
{
    cpuSetAX(640);
}

// System Reset
NORETURN void intr19(void)
{
    debug(debug_int, "INT 19: System reset!\n");
    exit(0);
}

// DOS/BIOS interface
void bios_routine(unsigned inum)
{
    if(inum == 0x21)
        intr21();
    else if(inum == 0x20)
        intr20();
    else if(inum == 0x22)
        intr22();
    else if(inum == 0x1A)
        intr1A();
    else if(inum == 0x19)
        intr19();
    else if(inum == 0x16)
        intr16();
    else if(inum == 0x10)
        intr10();
    else if(inum == 0x11)
        intr11();
    else if(inum == 0x12)
        intr12();
    else if(inum == 0x06)
    {
        uint16_t ip = cpuGetStack(0);
        uint16_t cs = cpuGetStack(2);
        print_error("error, unimplemented opcode %02X at cs:ip = %04X:%04X\n",
                    memory[cpuGetAddress(cs, ip)], cs, ip);
    }
    else if(inum == 0x28)
        intr28();
    else if(inum == 0x25)
        intr25();
    else if(inum == 0x29)
        intr29();
    else if(inum == 0x2A)
        intr2a();
    else if(inum == 0x2f)
        intr2f();
    else if(inum == 0x8)
        ; // Timer interrupt - nothing to do
    else if(inum == 0x9)
        keyb_handle_irq(); // Keyboard interrupt
    else
        debug(debug_int, "UNHANDLED INT %02x, AX=%04x\n", inum, cpuGetAX());
}


void init_bios_mem(void)
{
    // Some of those are also in video.c, we write a
    // default value here for programs that don't call
    // INT10 functions before reading.
    memory[0x413] = 0x80; // ram size: 640k
    memory[0x414] = 0x02; //
    // Store an "INT-19h" instruction in address FFFF:0000
    memory[0xFFFF0] = 0xCB;
    memory[0xFFFF1] = 0x19;
    // BIOS date at F000:FFF5
    memory[0xFFFF5] = 0x30;
    memory[0xFFFF6] = 0x31;
    memory[0xFFFF7] = 0x2F;
    memory[0xFFFF8] = 0x30;
    memory[0xFFFF9] = 0x31;
    memory[0xFFFFA] = 0x2F;
    memory[0xFFFFB] = 0x31;
    memory[0xFFFFC] = 0x37;

    update_timer();
}
