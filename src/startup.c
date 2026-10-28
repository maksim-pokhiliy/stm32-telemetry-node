#include <stdbool.h>
#include <stdint.h>

#include "stm32f411_regs.h"
#include "uart.h"

#define LED_TOGGLE_DELAY_MS 500u
#define UART_REPORT_DELAY_MS 1000u
#define SYSTICK_RELOAD_16MHZ_1MS 15999u

extern unsigned char STACK_TOP;
extern unsigned char DATA_FLASH_IMAGE_START;
extern unsigned char DATA_SRAM_START;
extern unsigned char DATA_SRAM_END;
extern unsigned char BSS_SRAM_START;
extern unsigned char BSS_SRAM_END;

volatile uint32_t systick_ms = 0u;

void Reset_Handler(void);
void Default_Handler(void);
void SysTick_Handler(void);

int main(void);

void Reset_Handler(void) {
  const uint32_t *flash_data = (const uint32_t *)&DATA_FLASH_IMAGE_START;
  uint32_t *sram_data = (uint32_t *)&DATA_SRAM_START;
  uint32_t *const sram_data_end = (uint32_t *)&DATA_SRAM_END;
  uint32_t *sram_bss = (uint32_t *)&BSS_SRAM_START;
  uint32_t *const sram_bss_end = (uint32_t *)&BSS_SRAM_END;

  while (sram_data < sram_data_end) {
    *sram_data = *flash_data;
    ++sram_data;
    ++flash_data;
  }

  while (sram_bss < sram_bss_end) {
    *sram_bss = 0u;
    ++sram_bss;
  }

  main();

  while (1) {
  }
}

void Default_Handler(void) {
  while (1) {
  }
}

void SysTick_Handler(void) { systick_ms++; }

int main(void) {
  volatile uint32_t *const gpioa_moder = (volatile uint32_t *)GPIOA_MODER_ADDR;
  volatile uint32_t *const gpioa_bsrr = (volatile uint32_t *)GPIOA_BSRR_ADDR;
  volatile uint32_t *const rcc_ahb1enr = (volatile uint32_t *)RCC_AHB1ENR_ADDR;

  volatile uint32_t *const stk_ctrl = (volatile uint32_t *)STK_CTRL_ADDR;
  volatile uint32_t *const stk_load = (volatile uint32_t *)STK_LOAD_ADDR;
  volatile uint32_t *const stk_val = (volatile uint32_t *)STK_VAL_ADDR;

  const uint32_t pin_5_mode_mask = GPIOA_MODER_PA5_MASK;
  const uint32_t pin_5_mode_value = GPIOA_MODER_PA5_OUTPUT;

  const uint32_t stk_ctrl_enable_mask = STK_CTRL_ENABLE_MASK;
  const uint32_t stk_ctrl_tickint_mask = STK_CTRL_TICKINT_MASK;
  const uint32_t stk_ctrl_clksource_mask = STK_CTRL_CLKSOURCE_MASK;

  *rcc_ahb1enr |= RCC_AHB1ENR_GPIOAEN_MASK;
  (void)*rcc_ahb1enr;

  *gpioa_moder = (*gpioa_moder & ~pin_5_mode_mask) | pin_5_mode_value;
  *gpioa_bsrr = GPIOA_BSRR_BS5;

  *stk_ctrl = 0u;
  *stk_load = SYSTICK_RELOAD_16MHZ_1MS;
  *stk_val = 0u;

  *stk_ctrl =
      stk_ctrl_enable_mask | stk_ctrl_tickint_mask | stk_ctrl_clksource_mask;

  uart_init();

  bool led_is_on = true;
  uint32_t last_led_toggle_ms = systick_ms;
  uint32_t last_uart_report_ms = systick_ms;

  while (1) {
    uint32_t now_ms = systick_ms;

    if (now_ms - last_led_toggle_ms >= LED_TOGGLE_DELAY_MS) {
      if (led_is_on) {
        *gpioa_bsrr = GPIOA_BSRR_BR5;
      } else {
        *gpioa_bsrr = GPIOA_BSRR_BS5;
      }

      led_is_on = !led_is_on;
      last_led_toggle_ms = now_ms;
    }

    if (now_ms - last_uart_report_ms >= UART_REPORT_DELAY_MS) {
      const char *hex_prefix_cursor = "uptime_s=0x";
      uint32_t now_s = now_ms / 1000u;

      uart_send_message(hex_prefix_cursor);
      uart_send_hex32(now_s);
      uart_send_message("\r\n");

      last_uart_report_ms = now_ms;
    }
  }
}

const uintptr_t VECTOR_TABLE[102] __attribute__((section(".vector_table"))) = {
    [0] = (uintptr_t)&STACK_TOP,
    [1] = (uintptr_t)&Reset_Handler,
    [2 ... 6] = (uintptr_t)&Default_Handler,
    [11] = (uintptr_t)&Default_Handler,
    [12] = (uintptr_t)&Default_Handler,
    [14] = (uintptr_t)&Default_Handler,
    [15] = (uintptr_t)&SysTick_Handler,

    [16 ... 34] = (uintptr_t)&Default_Handler,
    [39 ... 54] = (uintptr_t)&Default_Handler,
    [56 ... 58] = (uintptr_t)&Default_Handler,

    [63] = (uintptr_t)&Default_Handler,
    [65 ... 67] = (uintptr_t)&Default_Handler,
    [72 ... 76] = (uintptr_t)&Default_Handler,

    [83 ... 89] = (uintptr_t)&Default_Handler,
    [97] = (uintptr_t)&Default_Handler,
    [100 ... 101] = (uintptr_t)&Default_Handler,
};
