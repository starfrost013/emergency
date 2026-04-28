# Emergency
# (c) 2017-2026 dmsc 
# (c) 2026 starfrost

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(emergency
    
    main.c

    dos/codepage.c 
    dos/dos.c
    dos/dosnames.c
    dos/loader.c 

    pc/bios.c 
    pc/cpu.c
    pc/dbg.c
    pc/dis.c 
    pc/io.c
    pc/keyb.c
    pc/timer.c
    pc/video.c

    util/utils.c
)
