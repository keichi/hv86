#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include "uart.h"

UART::UART()
{
    thr = 0;
    rbr = 0;
    divisor = 0;
    ier = 0;
    iir = 0xc1;
    fcr = 0;
    lcr = 0;
    mcr = 0;
    lsr = 0;
    msr = 0;
    sr = 0;

    tcgetattr(STDIN_FILENO, &old_termios);
    new_termios = old_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    // Maybe O_NONBLOCK is better?
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

UART::~UART()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
}

void UART::write(uint32_t port, const uint32_t *value, uint8_t size)
{
    if (size != 1) {
        printf("PIT: PIT only supports single-byte R/W\n");
        return;
    }

    bool dlab = (lcr & 0x80) > 0;
    switch (port - UART_BASE_PORT) {
    case 0:
        if (dlab) {
            divisor = (divisor & 0xff00) | *value;
        } else {
            thr = *value;
        }
        break;
    case 1:
        if (dlab) {
            dlh = (divisor & 0x00ff) | (*value << 8);
        } else {
            ier = *value;
        }
        break;
    case 2:
        if (*value & UART_FCR_CLEAR_RX_FIFO) {
            std::queue<int> empty;
            std::swap(rx_buffer, empty);
        }
        fcr = *value & ~(UART_FCR_CLEAR_TX_FIFO | UART_FCR_CLEAR_RX_FIFO);
        break;
    case 3:
        lcr = *value;
        break;
    case 4:
        mcr = *value;
        break;
    case 5:
        printf("UART: LSR is read only\n");
        break;
    case 6:
        printf("UART: MSR is read only\n");
        break;
    case 7:
        sr = *value;
        write_sr(*value);
        break;
    }
}

void PIT::read(uint32_t port, uint32_t *value, uint8_t size)
{
    if (size != 1) {
        printf("PIT: PIT only supports single-byte R/W\n");
        return;
    }

    bool dlab = (lcr & 0x80) > 0;
    switch (port - UART_BASE_PORT) {
    case 0:
        if (dlab) {
            *value = divisor & 0x00ff;
        } else {
            *value = rbr;
        }
        break;
    case 1:
        if (dlab) {
            *value = (divisor & 0xff00) >> 8;
        } else {
            *value = ier;
        }
        break;
    case 2:
        *value = iir;
        break;
    case 3:
        *value = lcr;
        break;
    case 4:
        *value = mcr;
        break;
    case 5:
        *value = lsr;
        break;
    case 6:
        *value = msr;
        break;
    case 7:
        *value = sr;
        break;
    }
}

void UART::tx_chr(uint8_t c)
{
}

void UART::check_for_rx()
{
    int c;
    while ((c = getchar()) != EOF && rx_buffer.size() <= UART_FIFO_SIZE) {
        if (errno != EAGAIN) {
            break;
        }

        rx_buffer.push_back((uint8_t)c);
    }
}

uint8_t UART::rx_char()
{
    uint8_t c;

    if (rx_buffer.size() > 0) {
        c = rx_buffer.front();
        rx_buffer.pop();

        if (c == 0x03) {
            kill(0, SIGINT);
        }
    }
}

bool UART::poll_irq()
{
    int rx_trigger_size = 0;
    switch ((fcr & 0xc0) >> 6) {
        case 0:
            rx_trigger_size = 1;
            break;
        case 1:
            rx_trigger_size = 4;
            break;
        case 2:
            rx_trigger_size = 8;
            break;
        case 3:
            rx_trigger_size = 14;
            break;
    }

    if (rx_buffer.size() > rx_trigger_size) {
        // TODO IRQ
    }
}

