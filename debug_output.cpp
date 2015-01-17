#include "debug_output.h"

#include <stdio.h>

DebugOutput::DebugOutput()
{
}

DebugOutput::~DebugOutput()
{
}

void DebugOutput::write(uint32_t port, const uint32_t *value, uint8_t size)
{
    if (size != 1) {
        printf("DebugOutput: DebugOutput only supports single-byte R/W\n");
    }

    if (port == DEBUG_OUTPUT_BASE_PORT) {
        putchar(value & 0xff);
    }
}

void DebugOutput::read(uint32_t port, uint32_t *value, uint8_t size)
{
    printf("DebugOutput: This device is write only\n");
}

