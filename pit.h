#ifndef __PIT_H__
#define __PIT_H__

#include <stdint.h>
#include <mach/clock.h>
#include <mach/mach.h>

#include "io_device.h"

#define PIT_BASE_PORT   (0x40)
#define PIT_CLOCK_FREQ  (1193182)
#define PIT_CH_COUNT    (3)
#define PIT_S_IN_NS     (1000000000)
#define PIT_IRQ_CH      (0)

typedef enum {
    PIT_ACCESS_MODE_LATCH = 0,
    PIT_ACCESS_MODE_LOBYTE,
    PIT_ACCESS_MODE_HIBYTE,
    PIT_ACCESS_MODE_HILOBYTE,
} pit_access_mode_t;

typedef enum {
    PIT_OPERATING_MODE_0 = 0,
    PIT_OPERATING_MODE_2 = 2,
    PIT_OPERATING_MODE_3,
} pit_operating_mode_t;

class PIT : IODevice {
public:
    PIT();
    ~PIT();
    void write(uint32_t port, const uint32_t *value, uint8_t size);
    void read(uint32_t port, uint32_t *value, uint8_t size);
    bool poll_irq();
    void debug_status();
    // Output pin
    bool output[PIT_CH_COUNT];
private:
    void read_data(uint8_t channel, uint8_t *value);
    void write_data(uint8_t channel, uint8_t value);
    void write_control(uint8_t value);
    // Update internal state
    void tick();
    // Update counter internal state
    void tick_counter(int channel, uint64_t clocks);
    // Start counter
    void start_counter(int channel);
    // Get current real time in nanosec
    uint64_t get_real_time();

    // Counter starts from this value when reloaded
    uint16_t reload_values[PIT_CH_COUNT];
    // Current value for each counters
    uint16_t current_values[PIT_CH_COUNT];
    // Operating mode for each counter
    pit_operating_mode_t operating_modes[PIT_CH_COUNT];
    // Access mode for each counter
    pit_access_mode_t access_modes[PIT_CH_COUNT];
    // Flag if counter is running
    bool is_running[PIT_CH_COUNT];
    // Latched value
    uint16_t latched_values[PIT_CH_COUNT];
    // Byte index to access in next access (0: LOW byte, 1: HI byte)
    uint8_t access_bytes[PIT_CH_COUNT];

    // Time when the next clock will happen
    uint64_t next_clock_time;
    clock_serv_t clock_service;
};

#endif

