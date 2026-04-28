#pragma once

// bios.h: System BIOS 

#include <stdint.h>
#include <stdlib.h>
#include <dos/dos.h>
#include <emu.h>

#include <util/os.h>


void init_bios_mem(void);
void bios_routine(unsigned inum);
void intr11(void);
void intr12(void);
NORETURN void intr19(void);

extern interrupt_table_t bios_interrupts[];
static const int BIOS_LAST_INTERRUPT = 0x1F;