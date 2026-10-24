#include "stm32f411_regs.h"
#include <stdint.h>

extern unsigned char STACK_TOP;

extern unsigned char DATA_FLASH_IMAGE_START;

extern unsigned char DATA_SRAM_START;
extern unsigned char DATA_SRAM_END;

extern unsigned char BSS_SRAM_START;
extern unsigned char BSS_SRAM_END;

void Reset_Handler(void);
void Default_Handler(void);

int main(void);

void Reset_Handler(void) {
  const uint32_t *data_src = (const uint32_t *)&DATA_FLASH_IMAGE_START;

  uint32_t *data_dst = (uint32_t *)&DATA_SRAM_START;
  uint32_t *data_end = (uint32_t *)&DATA_SRAM_END;

  uint32_t *bss_dst = (uint32_t *)&BSS_SRAM_START;
  uint32_t *bss_end = (uint32_t *)&BSS_SRAM_END;

  while (data_dst < data_end) {
    *data_dst = *data_src;

    data_src++;
    data_dst++;
  }

  while (bss_dst < bss_end) {
    *bss_dst = 0;

    bss_dst++;
  }

  main();

  while (1) {
  }
}

void Default_Handler(void) {
  while (1) {
  }
}

int main(void) {
  volatile uint32_t *const rcc_ahb1enr_ptr =
      (volatile uint32_t *)RCC_AHB1ENR_ADDR;
  volatile uint32_t *const gpioa_moder_ptr =
      (volatile uint32_t *)GPIOA_MODER_ADDR;
  volatile uint32_t *const gpioa_bsrr_ptr =
      (volatile uint32_t *)GPIOA_BSRR_ADDR;

  *rcc_ahb1enr_ptr |= RCC_AHB1ENR_GPIOAEN_MASK;
  (void)*rcc_ahb1enr_ptr;

  *gpioa_moder_ptr &= ~GPIOA_MODER_MODER5_MASK;
  *gpioa_moder_ptr |= GPIOA_MODER_MODER5_OUTPUT;

  *gpioa_bsrr_ptr = GPIOA_BSRR_BS5;

  int led_state = 1;
  volatile uint32_t blink_counter = 0;

  while (1) {
    if (blink_counter >= 1000000u) {
      if (led_state == 1) {
        *gpioa_bsrr_ptr = GPIOA_BSRR_BR5;

        led_state = 0;
      } else if (led_state == 0) {
        *gpioa_bsrr_ptr = GPIOA_BSRR_BS5;

        led_state = 1;
      }

      blink_counter = 0;
    }

    blink_counter++;
  }
}

const uintptr_t VECTOR_TABLE[102] __attribute__((section(".vector_table"))) = {
    [0] = (uintptr_t)&STACK_TOP,
    [1] = (uintptr_t)&Reset_Handler,
    [2 ... 6] = (uintptr_t)&Default_Handler,
    [11] = (uintptr_t)&Default_Handler,
    [12] = (uintptr_t)&Default_Handler,
    [14 ... 15] = (uintptr_t)&Default_Handler,

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
