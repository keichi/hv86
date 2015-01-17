#ifndef __CMOS_H__
#define __CMOS_H__

#include <stdint.h>
#include <time.h>

#include "io_device.h"

#define CMOS_BASE_PORT  (0x70)
#define CMOS_FD_TYPE    (0x00)
#define CMOS_HD_TYPE    (0x00)
#define CMOS_EQUIPMENT  (0x01)
#define CMOS_BOOT_ORDER (0x123)

class CMOS : IODevice {
public:
    CMOS();
    ~CMOS();
    void write(uint32_t port, const uint32_t *value, uint8_t size);
    void read(uint32_t port, uint32_t *value, uint8_t size);
    void debug_status();
private:
    void write_address(uint8_t value);
    void read_data(uint8_t *value);
    void write_data(uint8_t value);
    // if is_binary_mode encode to BCD, else do nothing
    uint8_t encode(uint8_t value);
    // if is_binary_mode decode from BCD, else do nothing
    uint8_t decode(uint8_t value);
    uint8_t encode_hour(uint8_t value);
    uint8_t decode_hour(uint8_t value);
    struct tm *get_time_components();

    uint8_t address;
    bool periodic_interrupt_enabled;
    bool alarm_interrupt_enabled;
    bool update_interrupt_enabled;
    bool is_periodic_interrupt;
    bool is_alarm_interrupt;
    bool is_update_interrupt;
    bool is_binary_mode;
    bool is_24h_mode;
    uint8_t alarm_hour;
    uint8_t alarm_minute;
    uint8_t alarm_second;
    uint8_t periodic_interrupt_divider;
};

#endif

