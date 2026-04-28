#pragma once

#include <util/os.h>
#include <emu.h>

#include <stdint.h>
#include <stdio.h>

void init_dos(int argc, char **argv);
void dos_api_enter(int intnum);
NORETURN void intr20(void);
void intr21(void);
void intr2f(void);
NORETURN void intr22(void);
void intr25(void);
void intr28(void);
void intr29(void);
void intr2a(void);

extern interrupt_table_t dos_interrupts[];
static const int DOS_LAST_INTERRUPT = 0x2F;