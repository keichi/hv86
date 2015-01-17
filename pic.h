#ifndef __PIC_H__
#define __PIC_H__

#include "io_device.h"

#define PIC_BASE_PORT   (0x20)
#define PIC_IRQ_COUNT   (8)

typedef enum {
    PIC_STATUS_IDLE,
    PIC_STATUS_ICW2,
    PIC_STATUS_ICW3,
    PIC_STATUS_ICW4,
} pic_status_t;

class PIC : IODevice {
public:
    void write(uint32_t port, const uint32_t *value, uint8_t size);
    void read(uint32_t port, uint32_t *value, uint8_t size);
    void poll_irq();

    void push_irq(uint8_t irq_number);
    void connect_io_device(uint8_t irq_number, IODevice *device);
private:
    uint8_t read_command();
    uint8_t read_data();
    void write_command(uint8_t value);
    void write_data(uint8_t value);

    IODevice *devices[PIC_IRQ_COUNT];
    // Cascaded
    bool icw3_enabled;
    bool icw4_enabled;
    bool aeoi_enabled;
    bool rotation_enabled;
    // true: data register returns IRR, false: ISR
    bool data_is_irr;
    uint8_t interrupt_vector_address;
    pic_status_t status;
    uint8_t top_priority_irq;

    // Internal registers
    uint8_t iir;
    uint8_t imr;
    uint8_t isr;
};

#endif

