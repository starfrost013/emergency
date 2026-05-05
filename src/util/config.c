#include "config.h"

config_t config = {0};

// Initialise the configuration system
void config_init()
{
    config.stream = fopen(CONFIG_FILE_NAME, "r+"); // NOT BINARY

    // todo: 
    if (!config.stream)
    {
        // try and implicitly create it
        config.stream = fopen(CONFIG_FILE_NAME, "w+");

        if (!config.stream)
            emu_fatal("Failed to open configuration file at %s", CONFIG_FILE_NAME);
    }

}

char* config_get_string(const char* section, const char* name)
{

}

uint8_t config_get_uint8(const char* section, const char* name)
{

}

int8_t config_get_int8(const char* section, const char* name)
{
    return (int8_t)config_get_uint8(section, name);
}

uint16_t config_get_uint16(const char* section, const char* name)
{

}

int16_t config_get_int16(const char* section, const char* name)
{
    return (int8_t)config_get_uint16(section, name);
}

uint32_t config_get_uint32(const char* section, const char* name)
{

}

int32_t config_get_int32(const char* section, const char* name)
{
    return (int32_t)config_get_uint32(section, name);

}


void config_shutdown()
{

}
