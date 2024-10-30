#ifndef MINLIBC_H
#define MINLIBC_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>     // for atoi()

#define reg_uart_clkdiv    (*(volatile uint32_t*)0x02000010)
#define reg_uart_data      (*(volatile uint32_t*)0x02000014)
#define reg_time           (*(volatile uint32_t*)0x02000050)

#define DEBUG(...) uart_printf(__VA_ARGS__)

extern void delay(int ms);

extern void uart_init(int clkdiv);    // baudrate = clock_frequency / clkdiv. clock_frequency = 21505400
extern int uart_putchar(int c);
extern void uart_print_hex(uint32_t v);
extern void uart_print_hex_digits(uint32_t v, int n);
extern void uart_print_dec(int v);
extern int uart_print(const char *s);
extern int uart_printf(const char *fmt,...);
extern int uart_getchar();          // this stalls the entire CPU until a character is received

inline uint32_t time_millis() {
    return reg_time;
}

#endif
