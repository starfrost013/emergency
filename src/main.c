
#define _GNU_SOURCE

#include <pc/bios.h>
#include <pc/dbg.h>
#include <dos/dos.h>
#include <dos/dosnames.h>
#include <emu.h>
#include <pc/keyb.h>
#include <dos/loader.h>
#include <pc/timer.h>
#include <pc/video.h>
#include <util/os.h>

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

void emulator_update(void)
{
    debug(debug_int, "emu update cycle\n");
    cpuTriggerIRQ(0);
    update_timer();
    check_screen();
    update_keyb();
    fflush(stdout);
}

// Checks memory at exit: used for unit testings.
static uint8_t *chk_mem_arr = 0;
static unsigned chk_mem_len = 0;
static void check_exit_mem(void)
{
    if(!chk_mem_len || !chk_mem_arr)
        return;

    for(unsigned i = 0; i < chk_mem_len; i++)
    {
        if(chk_mem_arr[i] != memory[i])
        {
            fprintf(stderr, "%s: check memory: differ at byte %X, %02X != %02X\n",
                    prog_name, i, chk_mem_arr[i], memory[i]);
            break;
        }
    }
}

volatile int exit_cpu;
static void timer_alarm(int x)
{
    exit_cpu = 1;
}

NORETURN static void exit_handler(int x)
{
    exit(1);
}

int main(int argc, char **argv)
{
    int i;
    prog_name = argv[0];

    // Process command line options
    int bin_load_seg = 0, bin_load_ip = 0, bin_load_addr = -1;
    int skip_init_bios = 0;
    for(i = 1; i < argc; i++)
    {
        char flag;
        const char *opt = 0;
        char *ep;
        // Process options only *before* main program argument
        if(argv[i][0] != '-')
            break;
        flag = argv[i][1];
        // Check arguments:
        switch(flag)
        {
        case 'b':
        case 'r':
        case 'X':
            if(argv[i][2])
                opt = argv[i] + 2;
            else
            {
                if(i >= argc - 1)
                    print_usage_error("option '-%c' needs an argument.", flag);
                i++;
                opt = argv[i];
            }
        }
        // Process options
        switch(flag)
        {
        case 'h':
            print_usage();
        case 'v':
            print_version();
            exit(EXIT_SUCCESS);
        case 'b':
            bin_load_addr = strtol(opt, &ep, 0);
            if(*ep || bin_load_addr < 0 || bin_load_addr > 0xFFFF0)
                print_usage_error("binary load address '%s' invalid.", opt);
            bin_load_ip = bin_load_addr & 0x000FF;
            bin_load_seg = (bin_load_addr & 0xFFF00) >> 4;
            break;
        case 'r':
            bin_load_seg = strtol(opt, &ep, 0);
            if((*ep != 0 && *ep != ':') || bin_load_seg < 0 || bin_load_seg > 0xFFFF)
                print_usage_error("binary run segment '%s' invalid.", opt);
            if(*ep == 0)
            {
                bin_load_ip = bin_load_seg & 0x000F;
                bin_load_seg = bin_load_seg >> 4;
            }
            else
            {
                bin_load_ip = strtol(ep + 1, &ep, 0);
                if(*ep != 0 || bin_load_ip < 0 || bin_load_ip > 0xFFFF)
                    print_usage_error("binary run address '%s' invalid.", opt);
            }
            break;
        case 'X':
        {
            FILE *cf = fopen(opt, "rb");
            if(!cf)
                print_error("can't open '%s': %s\n", opt, strerror(errno));
            else
            {
                chk_mem_arr = malloc(1024 * 1024);
                chk_mem_len = fread(chk_mem_arr, 1, 1024 * 1024, cf);
                fprintf(stderr, "%s: will check %X bytes.\n", argv[0], chk_mem_len);
                skip_init_bios = 1;
                atexit(check_exit_mem);
            }
        }
        break;
        default:
            print_usage_error("invalid option '-%c'.", flag);
        }
    }

    // Move remaining options
    int j = 1;
    for(; i < argc; i++, j++)
        argv[j] = argv[i];
    argc = j;

    if(argc < 2)
        print_usage_error("program name expected.");

    // Init debug facilities
    init_debug(argv[1]);
    cpu_init();

    if(bin_load_addr >= 0)
    {
        dos_load_bin(argv[1], bin_load_addr);
        cpuSetIP(bin_load_ip);
        cpuSetCS(bin_load_seg);
        cpuSetDS(0);
        cpuSetES(0);
        cpuSetSP(0xFFFF);
        cpuSetSS(0);
    }
    else
        init_dos(argc - 1, argv + 1);

    struct sigaction timer_action, exit_action;
    exit_action.sa_handler = exit_handler;
    timer_action.sa_handler = timer_alarm;
    sigemptyset(&exit_action.sa_mask);
    sigemptyset(&timer_action.sa_mask);
    exit_action.sa_flags = timer_action.sa_flags = 0;
    sigaction(SIGALRM, &timer_action, NULL);
    // Install an exit handler to allow exit functions to run
    sigaction(SIGHUP, &exit_action, NULL);
    sigaction(SIGINT, &exit_action, NULL);
    sigaction(SIGQUIT, &exit_action, NULL);
    sigaction(SIGPIPE, &exit_action, NULL);
    sigaction(SIGTERM, &exit_action, NULL);
    struct itimerval itv;
    itv.it_interval.tv_sec = 0;
    itv.it_interval.tv_usec = 54925;
    itv.it_value.tv_sec = 0;
    itv.it_value.tv_usec = 54925;
    setitimer(ITIMER_REAL, &itv, 0);
    if(!skip_init_bios)
        init_bios_mem();
    video_init_mem();
    while(1)
    {
        exit_cpu = 0;
        cpu_execute();
        emulator_update();
    }
}
