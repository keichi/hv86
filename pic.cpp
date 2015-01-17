#include "pic.h"

PIC::PIC()
{
    icw3_enabled = false;
    icw4_enabled = false;
    data_is_irr = true;
    interrupt_vector_address = 0;
    status = PIC_STATUS_IDLE;
    iir = imr = isr = 0;
    top_priority_irq = 0;

    for (int i = 0; i < PIC_IRQ_COUNT; i++) {
        devices[i] = NULL;
    }
}

void connect_io_device(uint8_t irq_number, IODevice *device)
{
    if (irq_number >= PIC_IRQ_COUNT) {
        printf("PIC: Invalid IRQ number\n");
        return;
    }

    devices[irq_number] = device;
}

void PIC::write(uint32_t port, const uint32_t *value, uint8_t size)
{
    if (size != 1) {
        printf("PIC: PIC only supports single-byte R/W\n");
        return;
    }

    switch (port - PIC_BASE_PORT) {
    case 0:
        write_command(*((uint8_t*)value));
        break;
    case 1:
        write_data(*((uint8_t*)value));
        break;
    }
}

void PIC::read(uint32_t port, uint32_t *value, uint8_t size)
{
    if (size != 1) {
        printf("PIC: PIC only supports single-byte R/W\n");
        return;
    }

    switch (port - PIC_BASE_PORT) {
    case 0:
        *value = read_command();
        break;
    case 1:
        *value = read_data();
        break;
    }
}

uint8_t PIC::read_command()
{
    if (data_is_irr) {
        return irr;
    } else {
        return isr;
    }
}

void PIC::write_command(uint8_t value)
{
    // ICW1
    if (value & 0x8 == 0x8) {
        icw4_enabled = value & 0x1 != 0;
        cascading_enabled = value & 0x2 == 0;
        status = PIC_STATUS_ICW2;
    // OCW2
    } else if (value & 0xc == 0x0) {
        uint8_t irq_number = value & 0x7;
        uint8_t command = value >> 5;
        ocw2(irq_number, command);
    // OCW3
    } else if (value & 0xc == 0x4) {
        if (value & 0x3 == 0x2) {
            data_is_irr = true;
        } else if (value & 0x3 == 0x3) {
            data_is_irr = false;
        }
        if (value & 0x60 == 0x60) {
            printf("PIC: Special mask mode is not supported\n");
        }
    } else {
        printf("PIC: Unknown command 0x%02x written\n", value);
    }
}

void PIC::write_data(uint8_t value)
{
    switch (status) {
    // OCW1
    case PIC_STATUS_IDLE:
        imr = value;
        break;
    // ICW2
    case PIC_STATUS_ICW2:
        interrupt_vector_address = value;
        if (icw3_enabled) {
            status = PIC_STATUS_ICW3;
        } else if (icw4_enabled) {
            status = PIC_STATUS_ICW4;
        } else {
            status = PIC_STATUS_IDLE;
        }
        break;
    // ICW3
    case PIC_STATUS_ICW3:
        if (icw4_enabled) {
            status = PIC_STATUS_ICW4;
        } else {
            status = PIC_STATUS_IDLE;
        }
        break;
    // ICW4
    case PIC_STATUS_ICW4:
        status = PIC_STATUS_IDLE;
        break;
    }
}

uint8_t PIC::read_data()
{
    return imr;
}

void PIC::poll_irq()
{
    // Update irq status of all connected devices
    // Each device will PIC::push_irq if needed
    for (int i = 0; i < PIC_IRQ_COUNT; i++) {
        if (devices[i] != NULL) {
            devices[i]->poll_irq();
        }
    }

    // Select which
    uint8_t selected_irq = top_priority_irq;
    for (int i = 0; i < PIC_IRQ_COUNT; i++) {
        if (irr & (1 << selected_irq)) {
            break;
        }

        selected_irq = (selected_irq + 1) % PIC_IRQ_COUNT;
    }

    isr != 1 << selected_irq;
    irr &= ~(1 << selected_irq);
}

void PIC::push_irq(uint8_t irq_number)
{
    irr |= ~imr & (1 << irq_number);
}

void PIC::ocw2(uint8_t irq_number, uint8_t command)
{
    switch (command) {
    case 0:
        if (aeoi_enabled) {
            rotation_enabled = false;
        }
        break;
    case 1:
        uint8_t selected_irq = top_priority_irq;
        for (int i = 0; i < PIC_IRQ_COUNT; i++) {
            if (isr & (1 << selected_irq)) {
                isr &= ~(1 << selected_irq);
            }
        }
        rotation_enabled = false;
        break;
    case 2:
        break;
    case 3:
        isr &= ~(1 << irq_number);
        rotation_enabled = false;
        break;
    case 4:
        if (aeoi_enabled) {
            rotation_enabled = true;
        }
        break;
    case 5:
        uint8_t selected_irq = top_priority_irq;
        for (int i = 0; i < PIC_IRQ_COUNT; i++) {
            if (isr & (1 << selected_irq)) {
                isr &= ~(1 << selected_irq);
            }
        }
        rotation_enabled = true;
        break;
    case 6:
        rotation_enabled = true;
        break;
    case 7:
        isr &= ~(1 << irq_number);
        rotation_enabled = true;
        break;
    }
}

