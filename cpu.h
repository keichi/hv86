#ifndef __CPU_H__
#define __CPU_H__

class CPU {
public:
    void init();
    void connect_io_device(IODevice *device);
    void connect_memory(Memory *memory);

    void external_interrupt(uint8_t vector_number);
private:
    hv_vcpuid_t *vcpu;
};

#endif
