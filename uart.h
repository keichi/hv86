#ifndef __UART_H__
#define __UART_H__

#include <stdint.h>
#include <termios.h>
#include <queue>

#include "io_device.h"

#define UART_BASE_PORT          (0x03f8)
#define UART_FIFO_SIZE          (16)

#define UART_LSR_TX_FIN         (0x40)
#define UART_LSR_TX_BUF_EMPTY   (0x20)
#define UART_LSR_RX             (0x1)

#define UART_IIR_RX_TIMEOUT     (0xc)
#define UART_IIR_RX_LINE_STAT   (0x6)
#define UART_IIR_RX_DATA        (0x4)
#define UART_IIR_TX_DATA        (0x2)
#define UART_IIR_MODEM_STAT     (0x0)

#define UART_FCR_CLEAR_TX_FIFO  (0x4)
#define UART_FCR_CLEAR_RX_FIFO  (0x2)

class UART : IODevice {
public:
    UART();
    ~UART();
    void write(uint32_t port, const uint32_t *value, uint8_t size);
    void read(uint32_t port, uint32_t *value, uint8_t size);
    bool poll_irq();
    void debug_status();
private:
    std::queue<uint8_t> rx_buffer;

    // Transmitter Holding Buffer
    uint8_t thr;
    // Receiver Buffer
    uint8_t rbr;
    // Interrupt Enable Register
    uint8_t ier;
    // Interrupt Identification Register
    uint8_t iir;
    // FIFO Control Register
    uint8_t fcr;
    // Line Status Register
    uint8_t lsr;

    // Divisor Latch (doesn't matter; it's an emulation)
    uint8_t divisor;
    // Line Control Register (nothing important other than DLAB)
    uint8_t lcr;
    // Modem Control Register (nothing important here);
    uint8_t mcr;
    // Modem Status Register (nothing important here)
    uint8_t msr;
    // Scratch Register (we emulate this just in case)
    uint8_t sr;

    struct termios old_termios;
    struct termios new_termios;
};

#endif

