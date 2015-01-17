#include "cmos.h"

#include <stdio.h>

CMOS::CMOS()
{
    address = 0;
    
    periodic_interrupt_enabled = false;
    alarm_interrupt_enabled = false;
    update_interrupt_enabled = false;
    is_periodic_interrupt = false;
    is_alarm_interrupt = false;
    is_update_interrupt = false;

    is_binary_mode = true;
    is_24h_mode = true;

    alarm_hour = 0;
    alarm_minute = 0;
    alarm_second = 0;
    
    periodic_interrupt_divider = 6;
}

CMOS::~CMOS()
{
}

void CMOS::write(uint32_t port, const uint32_t *value, uint8_t size)
{
    if (size != 1) {
        printf("CMOS: CMOS only supports single-byte R/W\n");
        return;
    }

    switch (port - CMOS_BASE_PORT) {
    case 0:
        write_address(*((uint8_t*)value));
        break;
    case 1:
        write_data(*((uint8_t*)value));
        break;
    }
}

void CMOS::read(uint32_t port, uint32_t *value, uint8_t size)
{
    if (size != 1) {
        printf("CMOS: CMOS only supports single-byte R/W\n");
        return;
    }

    switch (port - CMOS_BASE_PORT) {
    case 0:
        printf("CMOS: Address register is write only\n");
        break;
    case 1:
        read_data((uint8_t*)value);
        break;
    }
}

void CMOS::write_address(uint8_t value)
{
    address = value;
}

void CMOS::write_data(uint8_t value)
{
    switch(address) {
    // Alarm seconds
    case 0x01:
        alarm_second = decode(value);
        break;
    // Alarm minutes
    case 0x03:
        alarm_minute = decode(value);
        break;
    // Alarm hours
    case 0x05:
        alarm_hour = decode_hour(value);
        break;
    // RTC status register A
    case 0x0a:
        periodic_interrupt_divider = value & 0xf;
        break;
    // RTC status register B 
    case 0x0b:
        periodic_interrupt_enabled = (value & (1 << 6)) > 0;
        alarm_interrupt_enabled = (value & (1 << 5)) > 0;
        update_interrupt_enabled = (value & (1 << 4)) > 0;
        is_binary_mode = (value & (1 << 2)) > 0;
        is_24h_mode = (value & (1 << 1)) > 0;
        break;
    default:
        break; 
    }
}

void CMOS::read_data(uint8_t *value)
{
    switch(address) {
    // RTC seconds
    case 0x00:
        *value = encode(get_time_components()->tm_sec);
        break;
    // Alarm seconds
    case 0x01:
        *value = encode(alarm_second);
        break;
    // RTC minutes
    case 0x02:
        *value = encode(get_time_components()->tm_min);
        break;
    // Alarm minutes
    case 0x03:
        *value = encode(alarm_minute);
        break;
    // RTC hours
    case 0x04:
        *value = encode_hour(get_time_components()->tm_hour);
        break;
    // Alarm hours
    case 0x05:
        *value = encode_hour(alarm_hour);
        break;
    // RTC day of week
    case 0x06:
        *value = encode(get_time_components()->tm_wday);
        break;
    // RTC day
    case 0x07:
        *value = encode(get_time_components()->tm_mday);
        break;
    // RTC month
    case 0x08:
        *value = encode(get_time_components()->tm_mon + 1);
        break;
    // RTC year
    case 0x09:
        *value = encode((get_time_components()->tm_year + 1900) % 100);
        break;
    // RTC status register A
    case 0x0a:
        *value = 0x20 | periodic_interrupt_divider;
        break;
    // RTC status register B 
    case 0x0b:
        *value =
            (periodic_interrupt_enabled ? 1 << 6 : 0) |
            (alarm_interrupt_enabled ? 1 << 5 : 0) |
            (update_interrupt_enabled ? 1 << 4 : 0) |
            (is_binary_mode ? 1 << 2 : 0) |
            (is_24h_mode ? 1 << 1 : 0); 
        break;
    // RTC status register C
    case 0x0c: {
        bool irqf = (periodic_interrupt_enabled && is_periodic_interrupt)
            || (alarm_interrupt_enabled && is_alarm_interrupt)
            || (update_interrupt_enabled && is_update_interrupt);
        *value =
            (irqf ? 1 << 7 : 0) |
            (is_periodic_interrupt ? 1 << 6 : 1) |
            (is_alarm_interrupt ? 1 << 5 : 1) |
            (is_update_interrupt ? 1 << 4 : 1);
    }
        break;
    // RTC status register D (CMOS battery status)
    case 0x0d:
        *value = (1 << 7);
        break;
    // Diagnostic status
    case 0x0e:
        *value = 0;
        break;
    // CMOS shutdown status
    case 0x0f:
        *value = 0;
        break;
    // Floppy disk drive type
    case 0x10:
        *value = CMOS_FD_TYPE;
        break;
    // Hard disk type
    case 0x12:
        *value = CMOS_HD_TYPE;
        break;
    // Installed equipment
    case 0x14:
        *value = CMOS_EQUIPMENT;
        break;
    case 0x32:
        *value = encode((get_time_components()->tm_year + 1900) / 100);
        break;
    case 0x38:
        // Boot order used in SeaBIOS 
        // https://github.com/copy/v86/blob/master/src/rtc.js
        *value = ((CMOS_BOOT_ORDER >> 4) & 0xf0) | 1;
        break;
    case 0x3d:
        *value = CMOS_BOOT_ORDER & 0xff;
        break;
    case 0x5b:
    case 0x5c:
    case 0x5d:
        *value = 0;
        break;
    }
}

uint8_t CMOS::encode(uint8_t value)
{
    if (is_binary_mode) {
        return value;
    }

    return ((value / 10) & 0xf) << 4 | ((value % 10) & 0xf);
}

uint8_t CMOS::decode(uint8_t value)
{
    if (is_binary_mode) {
        return value;
    }

    return ((value & 0xf0) >> 4) * 10 + (value & 0xf); 
}

uint8_t CMOS::encode_hour(uint8_t value)
{
    if (is_24h_mode || value <= 12) {
        return encode(value);
    }

    return encode((value % 12) | 0x80);
}

uint8_t CMOS::decode_hour(uint8_t value)
{
    if (is_24h_mode || !(value & 0x80)) {
        return decode(value);
    }

    return decode((value & ~0x80) + 12);
}

struct tm *CMOS::get_time_components()
{
    time_t time_since_epoch;
    struct tm *calendar;
    
    time(&time_since_epoch);

    return localtime(&time_since_epoch);
}

void CMOS::debug_status()
{
    printf("------------------------------\n");
    printf("CMOS:\n");
    printf("Address: 0x%02x\n", address);
    printf("Periodic Interrupt: %s %s\n",
            periodic_interrupt_enabled ? "enabled" : "disabled",
            is_periodic_interrupt ? "happened" : "");
    printf("Alarm Interrupt: %s %s\n",
            alarm_interrupt_enabled ? "enabled" : "disabled",
            is_alarm_interrupt ? "happened" : "");
    printf("Update Interrupt: %s %s\n",
            update_interrupt_enabled ? "enabled" : "disabled",
            is_update_interrupt ? "happened" : "");
    printf("Modes: %s %s\n",
            is_binary_mode ? "Binary" : "BCD",
            is_24h_mode ? "24h" : "12h");
    printf("Alarm time: %02d:%02d:%02d\n",
            alarm_hour, alarm_minute, alarm_second);
    printf("Periodic interrupt frequency: %dHz\n",
            32768 / periodic_interrupt_divider);
    printf("------------------------------\n");
}

