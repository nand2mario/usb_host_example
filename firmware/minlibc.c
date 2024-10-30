#include <stdarg.h>
#include "minlibc.h"


int uart_putchar(int c);
void putchar(char c) {
    uart_putchar(c);
}

int _putchar(int c, int uart) {
   if (uart)
      uart_putchar(c);
   else
      putchar(c);

   return c;
}

int print(const char *p)
{
	while (*p)
		putchar(*(p++));
   return 0;
}
int uart_print(const char *p);
int _print(const char *p, int uart) {
   if (uart)
      uart_print(p);
   else
      print(p);

   return 0;
}

void _print_hex_digits(uint32_t val, int nbdigits, int uart) {
   for (int i = (4*nbdigits)-4; i >= 0; i -= 4) {
      _putchar("0123456789ABCDEF"[(val >> i) % 16], uart);
   }
}
void print_hex_digits(uint32_t val, int nbdigits) {
   _print_hex_digits(val, nbdigits, 0);
}
void uart_print_hex_digits(uint32_t val, int ndigits) {
   _print_hex_digits(val, ndigits, 1);
}

void _print_hex(uint32_t val, int uart) {
   _print_hex_digits(val, 8, uart);
}
void print_hex(uint32_t val) {
   _print_hex(val, 0);
}
void uart_print_hex(uint32_t val) {
   _print_hex(val, 1);
}

void _print_dec(int val, int uart) {
   char buffer[255];
   char *p = buffer;
   if(val < 0) {
      _putchar('-', uart);
      _print_dec(-val, uart);
      return;
   }
   while (val || p == buffer) {
      *(p++) = val % 10;
      val = val / 10;
   }
   while (p != buffer) {
      _putchar('0' + *(--p), uart);
   }
}
void print_dec(int val) {
   _print_dec(val, 0);
}
void uart_print_dec(int val) {
   _print_dec(val, 1);
}

int _printf(const char *fmt, va_list ap, int uart) {
    for(;*fmt;fmt++) {
        if(*fmt=='%') {
            fmt++;
                 if(*fmt=='s') _print(va_arg(ap,char *), uart);
            else if(*fmt=='x') _print_hex(va_arg(ap,int), uart);
            else if(*fmt=='d') _print_dec(va_arg(ap,int), uart);
            else if(*fmt=='c') _putchar(va_arg(ap,int), uart);	   
            else if(*fmt=='0' && *(fmt+1) == '2' && *(fmt+2) == 'x') {     // %02x
               _print_hex_digits(va_arg(ap,int), 2, uart);
               fmt += 2;
            } 
            else if(*fmt=='0' && *(fmt+1) == '4' && *(fmt+2) == 'x') {     // %04x
               _print_hex_digits(va_arg(ap,int), 4, uart);	      
               fmt += 2;
            } 
            else _putchar(*fmt, uart);
        } else 
            _putchar(*fmt, uart);
    }
    return 0;
}


int printf(const char *fmt,...)
{
   va_list ap;
   va_start(ap, fmt);
   _printf(fmt, ap, 0);
   va_end(ap);
   return 0;
}

void uart_init(int clkdiv) {
   reg_uart_clkdiv = clkdiv;
}

int uart_putchar(int c) {
   reg_uart_data = c;
   return c;
}

int uart_getchar() {
   return reg_uart_data;
}

int uart_print(const char *s) {
	while (*s)
		_putchar(*(s++), 1);   
   return 0;
}

int uart_printf(const char *fmt,...) {
   va_list ap;
   va_start(ap, fmt);
   _printf(fmt, ap, 1);
   va_end(ap);
   return 0;   
}

void delay(int ms) {
   int t0 = time_millis();
   while (time_millis() - t0 < ms) {}
}

