#pragma once

#include <stdint.h>
#include "os.h"

void init_bios_mem(void);
void bios_routine(unsigned inum);
void intr11(void);
void intr12(void);
NORETURN void intr19(void);
