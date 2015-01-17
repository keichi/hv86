#include "pit.h"

#include <mach/clock.h>
#include <mach/mach.h>
#include <stdio.h>

PIT::PIT()
{
    for (int i = 0; i < PIT_CH_COUNT; i++) {
        reload_values[i] = 0;
        current_values[i] = 0;
        output[i] = false;
        operating_modes[i] = PIT_OPERATING_MODE_0;
        access_modes[i] = PIT_ACCESS_MODE_LOBYTE;
        is_running[i] = false;
        latched_values[i] = 0;
        access_bytes[i] = 0;
    }

    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &clock_service);
    next_clock_time = get_real_time();
}

PIT::~PIT()
{
    mach_port_deallocate(mach_task_self(), clock_service);
}

void PIT::write(uint32_t port, const uint32_t *value, uint8_t size)
{
    if (size != 1) {
        printf("PIT: PIT only supports single-byte R/W\n");
        return;
    }

    switch (port - PIT_BASE_PORT) {
    case 0:
        write_data(0, *((uint8_t*)value));
        break;
    case 1:
        write_data(1, *((uint8_t*)value));
        break;
    case 2:
        write_data(2, *((uint8_t*)value));
        break;
    case 3:
        write_control(*((uint8_t*)value));
        break;
    }
}

void PIT::read(uint32_t port, uint32_t *value, uint8_t size)
{
    if (size != 1) {
        printf("PIT: PIT only supports single-byte R/W\n");
        return;
    }

    switch (port - PIT_BASE_PORT) {
    case 0:
        read_data(0, (uint8_t*)value);
        break;
    case 1:
        read_data(1, (uint8_t*)value);
        break;
    case 2:
        read_data(2, (uint8_t*)value);
        break;
    case 3:
        printf("PIT: Control register is write only\n");
        break;
    }
}

bool PIT::poll_irq()
{
    tick();
}

void PIT::tick()
{
    uint64_t current_time = get_real_time();
    // Elapsed time [ns] since the time event should have been occured
    uint64_t elapsed_time = current_time - next_clock_time;
    // Elapsed time counverted to clock counts
    uint64_t clocks = elapsed_time * PIT_CLOCK_FREQ / PIT_S_IN_NS;

    if (current_time <= next_clock_time)
    {
        return;
    }

    for (int i = 0; i < PIT_CH_COUNT; i++) {
        if (!is_running[i]) {
            continue;
        }
        tick_counter(i, clocks);
    }

    next_clock_time = current_time + elapsed_time;
}

void PIT::tick_counter(int ch, uint64_t clocks)
{
    uint16_t steps = clocks % reload_values[ch];

    if (current_values[ch] <= steps) {
        // Reload counter value
        current_values[ch] = reload_values[ch] + (current_values[ch] - steps)
            % reload_values[ch];

        if (ch == PIT_IRQ_CH) {
            // TODO do IRQ
        }

        switch (operating_modes[ch]) {
        case PIT_OPERATING_MODE_0:
            output[ch] = true;
            // Counter should continue counting, but we ignore it
            is_running[ch] = false;
            break;
        case PIT_OPERATING_MODE_2:
            // Output should go LOW for 1 CLK, but we just ignore it
            break;
        case PIT_OPERATING_MODE_3:
            output[ch] = !output[ch];
            break;
        }
    } else {
        switch (operating_modes[ch]) {
        case PIT_OPERATING_MODE_0:
            break;
        case PIT_OPERATING_MODE_2:
            break;
        case PIT_OPERATING_MODE_3:
            if (current_values[ch] > reload_values[ch] / 2
                    && current_values[ch] - steps <= reload_values[ch] / 2) {
                output[ch] = !output[ch];
            }
            break;
        }
        current_values[ch] -= steps;
    }
}

uint64_t PIT::get_real_time()
{
    mach_timespec_t ts;
    clock_get_time(clock_service, &ts);

    return (uint64_t)ts.tv_sec * PIT_S_IN_NS + (uint64_t)ts.tv_nsec;
}

void PIT::write_control(uint8_t value)
{
    uint8_t bcd_binary_mode = value & 0x1;
    uint8_t operating_mode = (value >> 1) & 0x7;
    uint8_t access_mode = (value >> 4) & 0x3;
    uint8_t channel = (value >> 6) & 0x3;

    if (bcd_binary_mode != 0 || channel >= PIT_CH_COUNT || operating_mode == 1
        || operating_mode > 3) {
        printf("PIT: Unsopported feature\n");
        return;
    }

    access_modes[channel] = (pit_access_mode_t)access_mode;

    /* if access mode is latch count */
    if (access_mode == PIT_ACCESS_MODE_LATCH) {
        tick();
        latched_values[channel] = current_values[channel];
        return;
    } else {
        latched_values[channel] = 0;
    }

    operating_modes[channel] = (pit_operating_mode_t)operating_mode;
    if (access_mode == PIT_ACCESS_MODE_HILOBYTE) {
        access_bytes[channel] = 0;
    }
    reload_values[channel] = 0;
    current_values[channel] = 0;
    output[channel] = true;
    is_running[channel] = false;
}

void PIT::read_data(uint8_t channel, uint8_t *value)
{
    tick();
    uint16_t counter = current_values[channel];

    switch (access_modes[channel]) {
    case PIT_ACCESS_MODE_LOBYTE:
        *value = counter & 0xff;
        break;
    case PIT_ACCESS_MODE_HIBYTE:
        *value = counter >> 8;
        break;
    case PIT_ACCESS_MODE_HILOBYTE:
        if (access_bytes[channel] == 0) {
            *value = counter & 0xff;
            access_bytes[channel] = 1;
        } else if (access_bytes[channel] == 1) {
            *value = counter >> 8;
            access_bytes[channel] = 0;
        }
        break;
    case PIT_ACCESS_MODE_LATCH:
        if (access_bytes[channel] == 0) {
            *value = latched_values[channel] & 0xff;
            access_bytes[channel] = 1;
        } else if (access_bytes[channel] == 1) {
            *value = latched_values[channel] >> 8;
            access_bytes[channel] = 0;
            latched_values[channel] = 0;
        }
        break;
    }
}

void PIT::write_data(uint8_t channel, uint8_t value)
{
    uint16_t *reload_value;
    reload_value  = &reload_values[channel];

    switch (access_modes[channel]) {
    case PIT_ACCESS_MODE_LOBYTE:
        *reload_value = (*reload_value & 0xff00) | value;
        start_counter(channel);
        break;
    case PIT_ACCESS_MODE_HIBYTE:
        *reload_value = (*reload_value & 0xff) | value << 8;
        start_counter(channel);
        break;
    case PIT_ACCESS_MODE_LATCH:
    case PIT_ACCESS_MODE_HILOBYTE:
        if (access_bytes[channel] == 0) {
            *reload_value = (*reload_value & 0xff00) | value;
            access_bytes[channel] = 1;
        } else if (access_bytes[channel] == 1) {
            *reload_value = (*reload_value & 0xff) | value << 8;
            access_bytes[channel] = 0;
            start_counter(channel);
        }
        break;
    }
}

void PIT::start_counter(int channel)
{
    if (is_running[channel]) {
        return;
    }

    current_values[channel] = reload_values[channel];
    is_running[channel] = true;

    switch (operating_modes[channel]) {
    case PIT_OPERATING_MODE_0:
        output[channel] = false;
        break;
    case PIT_OPERATING_MODE_2:
        output[channel] = true;
        break;
    case PIT_OPERATING_MODE_3:
        output[channel] = true;
        break;
    }
}

void PIT::debug_status()
{
    printf("------------------------------\n");
    for (int i = 0; i < PIT_CH_COUNT; i++) {
        printf("PIT: Status for counter %d\n", i);
        printf("reload: 0x%04x, current: 0x%04x, latched: 0x%04x\n",
                reload_values[i],
                current_values[i],
                latched_values[i]);
        printf("output: %s, operating mode: %d\n",
                output[i] ? "HIGH" : "LOW",
                operating_modes[i]);
        printf("access mode: %d, access byte: %d, %s\n",
                access_modes[i],
                access_bytes[i],
                is_running[i] ? "running" : "NOT running");
        printf("------------------------------\n");
    }
}
