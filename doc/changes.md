# Changelog

**Emergency 1.0 Changes from emu2-2021.01+9d8698d06e67359bc8b230c2d19d5b1503e60819:**

* Moved to CMake.
* Pulled all non-program-lifetime related functions out of main.c to the new io.c, bios.c and also loader.c and dos.c
* Rebranded to Emergency!
* Implemented "verbose" debug that spams a bunch of nonsense.
* Split DOS and BIOS away from each other (DOS has its own interrupt function)
* Use static tables for interrupts to reduce code spam