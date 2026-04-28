/* bios.c: System BIOS */

#include <pc/bios.h>
#include <pc/dbg.h>
#include <dos/dos.h>
#include <emu.h>
#include <pc/keyb.h>
#include <pc/timer.h>
#include <pc/video.h>

void intr06(void)
{
    uint16_t ip = cpuGetStack(0);
    uint16_t cs = cpuGetStack(2);
    print_error("error, unimplemented opcode %02X at cs:ip = %04X:%04X\n",
                memory[cpuGetAddress(cs, ip)], cs, ip);
}

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
    if (inum <= BIOS_LAST_INTERRUPT && bios_interrupts[inum].func != NULL)
        bios_interrupts[inum].func();
    else
        debug(debug_int, "UNHANDLED BIOS INT %02x, AX=%04x\n", inum, cpuGetAX());
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

// BIOS - INTERRUPT TABLE
interrupt_table_t bios_interrupts[] =
{
    NULL, NULL, NULL, NULL, 
    NULL, intr06, NULL, NULL,
    keyb_handle_irq, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL,
      
    intr10, intr11, intr12, NULL,
    NULL, NULL, intr16, NULL,
    intr19, intr1A, NULL, NULL,
    NULL, NULL, NULL, NULL,

};
