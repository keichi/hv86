#ifndef __MEMORY_H__
#define __MEMORY_H__

class Memory {
public:
    Memory(size_t size);
    ~Memory();
    void load_bios(const char *filename);
    void load_vga_bios(const char *filename);
private:
    void *memory;
}

#endif
