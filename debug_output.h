#ifndef __DEBUG_OUTPUT_H__
#define __DEBUG_OUTPUT_H__

#include <stdint.h>

#define DEBUG_OUTPUT_BASE_PORT   (0x402)

class DebugOutput {
public:
    DebugOutput();
    ~DebugOutput();
    void write(uint32_t port, const uint32_t *value, uint8_t size);
    void read(uint32_t port, uint32_t *value, uint8_t size);
};

#endif

