#include <stdio.h>
#include <malloc.h>

#include "memory.h"

Memory::Memory(size_t size)
{
    memory = valloc(size);
}

Memory::~Memory()
{
    free(memory);
}

void Memory::load_file(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        printf("Memory: Failed to load file '%s'\n", filename);
        return;
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);

    fseek(fp, 0, SEEK_SET);
    fread(memory, size, 1, fp);

    fclose(fp);
}

void Memory::load_bios(const char *filename)
{
    load_file(filename, )
}

void Memory::load_vga_bios(const char *filename)
{
}

