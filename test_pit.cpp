#include "pit.h"
#include <stdio.h>

int main()
{
    PIT pit;
    uint32_t eax = 0;

    // Start counter
    eax = 0xb6;
    // eax = 0xb0;
    pit.write(0x43, &eax, 1);
    eax = 0x98;
    pit.write(0x42, &eax, 1);
    eax = 0x0a;
    pit.write(0x42, &eax, 1);

    for (int i = 0; i < 10000; i++) {
        eax = 0x80;
        pit.write(0x43, &eax, 1);
        pit.read(0x42, &eax, 1);
        pit.read(0x42, (uint32_t*)((uint8_t*)&eax + 1), 1);

        // printf("count: 0x%04x, output: %s\n", eax, pit.output[2] ? "HIGH": "LOW");
        printf("%d\n", eax);
    }

    pit.debug_status();

    return 0;
}
