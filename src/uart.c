#include "uart.h"
#include "stm32f411_regs.h"
#include <stdint.h>

#define USART2_BRR_16MHZ_115200 0x008Bu
#define UART_UINT32_MAX_DIGITS 10u

void uart_init(void) {
  volatile uint32_t *const rcc_ahb1enr = (volatile uint32_t *)RCC_AHB1ENR_ADDR;
  volatile uint32_t *const rcc_apb1enr = (volatile uint32_t *)RCC_APB1ENR_ADDR;
  volatile uint32_t *const gpioa_moder = (volatile uint32_t *)GPIOA_MODER_ADDR;
  volatile uint32_t *const gpioa_afrl = (volatile uint32_t *)GPIOA_AFRL_ADDR;
  volatile uint32_t *const usart2_brr = (volatile uint32_t *)USART2_BRR_ADDR;
  volatile uint32_t *const usart2_cr1 = (volatile uint32_t *)USART2_CR1_ADDR;

  const uint32_t pins_mode_mask = GPIOA_MODER_PA2_MASK | GPIOA_MODER_PA3_MASK;
  const uint32_t pins_mode_value = GPIOA_MODER_PA2_AF | GPIOA_MODER_PA3_AF;

  const uint32_t alternate_function_mask =
      GPIOA_AFRL_PA2_MASK | GPIOA_AFRL_PA3_MASK;

  const uint32_t alternate_function_value =
      GPIOA_AFRL_PA2_AF7 | GPIOA_AFRL_PA3_AF7;

  *rcc_ahb1enr |= RCC_AHB1ENR_GPIOAEN_MASK;
  (void)*rcc_ahb1enr;

  *rcc_apb1enr |= RCC_APB1ENR_USART2EN_MASK;
  (void)*rcc_apb1enr;

  *gpioa_afrl =
      (*gpioa_afrl & ~alternate_function_mask) | alternate_function_value;

  *gpioa_moder = (*gpioa_moder & ~pins_mode_mask) | pins_mode_value;

  *usart2_cr1 |= USART_CR1_UE;
  *usart2_brr = USART2_BRR_16MHZ_115200;
  *usart2_cr1 |= USART_CR1_TE;
}

void uart_send_byte(uint8_t byte) {
  volatile uint32_t *const usart2_sr = (volatile uint32_t *)USART2_SR_ADDR;
  volatile uint32_t *const usart2_dr = (volatile uint32_t *)USART2_DR_ADDR;

  while ((*usart2_sr & USART_SR_TXE) == 0u) {
  }

  *usart2_dr = byte;
}

void uart_send_message(const char *msg_cursor) {
  volatile uint32_t *const usart2_sr = (volatile uint32_t *)USART2_SR_ADDR;

  while (*msg_cursor != '\0') {
    uart_send_byte(*msg_cursor);
    ++msg_cursor;
  }

  while ((*usart2_sr & USART_SR_TC) == 0u) {
  }
}

void uart_send_hex32(uint32_t value) {
  for (int i = 7; i >= 0; i--) {
    uint32_t nibble = (value >> (i * 4)) & 0xF;
    uint8_t ascii_symbol;

    if (nibble < 10) {
      ascii_symbol = (uint8_t)('0' + nibble);
    } else {
      ascii_symbol = (uint8_t)('A' + nibble - 10);
    }

    uart_send_byte(ascii_symbol);
  }
}

void uart_send_uint32(uint32_t value) {
  uint8_t digits[UART_UINT32_MAX_DIGITS];
  uint32_t digit_count = 0u;

  do {
    digits[digit_count] = (uint8_t)('0' + (value % 10u));
    digit_count++;

    value /= 10u;
  } while (value != 0u);

  while (digit_count > 0u) {
    digit_count--;

    uart_send_byte(digits[digit_count]);
  }
}

void uart_send_int32(int32_t value) {
  uint32_t magnitude = (uint32_t)value;

  if (value < 0) {
    magnitude = 0u - magnitude;

    uart_send_byte('-');
  }

  uart_send_uint32(magnitude);
}

void uart_send_value(const char *label, uint32_t value) {
  uart_send_message(label);
  uart_send_hex32(value);
}

void uart_send_decimal(const char *label, uint32_t value) {
  uart_send_message(label);
  uart_send_uint32(value);
}
