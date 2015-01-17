#ifndef __IO_DEVICE_H__
#define __IO_DEVICE_H__

class IODevice {
public:
    virtual ~IODevice() {}
    virtual void write(uint32_t port, const uint32_t *value, uint8_t size) = 0;
    virtual void read(uint32_t port, uint32_t *value, uint8_t size) = 0;
    virtual bool poll_irq() {};
};

#endif

