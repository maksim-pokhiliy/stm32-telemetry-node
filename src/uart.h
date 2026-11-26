#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_init(void);
void uart_send_byte(uint8_t byte);
void uart_send_message(const char *msg_cursor);
void uart_send_hex32(uint32_t value);
void uart_send_int32(int32_t value);
void uart_send_value(const char *label, uint32_t value);
void uart_send_decimal(const char *label, uint32_t value);

#endif
