#include "stm32f411_regs.h"
#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* External symbols defined by the linker script                              */
/* -------------------------------------------------------------------------- */
extern unsigned char STACK_TOP; // Top of the stack

extern unsigned char
    DATA_FLASH_IMAGE_START; // Start of initialized data image in Flash

extern unsigned char DATA_SRAM_START; // Start of .data section in SRAM
extern unsigned char DATA_SRAM_END;   // End of .data section in SRAM

extern unsigned char BSS_SRAM_START; // Start of .bss section in SRAM
extern unsigned char BSS_SRAM_END;   // End of .bss section in SRAM

/* -------------------------------------------------------------------------- */
/* Function declarations                                                      */
/* -------------------------------------------------------------------------- */
void Reset_Handler(void);
void Default_Handler(void);

int main(void);

/* -------------------------------------------------------------------------- */
/* Reset_Handler - Entry point after MCU reset                                */
/* Performs minimal C runtime initialization before calling main()            */
/* -------------------------------------------------------------------------- */
void Reset_Handler(void) {
  /* Copy initialized data (.data) from Flash to SRAM */
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

  /* Zero-initialize the .bss section in SRAM */
  while (bss_dst < bss_end) {
    *bss_dst = 0;

    bss_dst++;
  }

  /* Jump to main application */
  main();

  /* If main() ever returns, stay here forever */
  while (1) {
  }
}

/* -------------------------------------------------------------------------- */
/* Default_Handler - Catch-all handler for unused interrupts                  */
/* -------------------------------------------------------------------------- */
void Default_Handler(void) {
  while (1) {
  }
}

/* -------------------------------------------------------------------------- */
/* main - Application entry point                                             */
/* -------------------------------------------------------------------------- */
int main(void) {
  /* Pointers to peripheral registers */
  volatile uint32_t *const rcc_ahb1enr_ptr = (volatile uint32_t *)
      RCC_AHB1ENR_ADDR; // RCC AHB1 peripheral clock enable register

  volatile uint32_t *const rcc_apb1enr_ptr = (volatile uint32_t *)
      RCC_APB1ENR_ADDR; // RCC APB1 peripheral clock enable register

  volatile uint32_t *const gpioa_moder_ptr =
      (volatile uint32_t *)GPIOA_MODER_ADDR; // GPIOA mode register

  volatile uint32_t *const gpioa_bsrr_ptr =
      (volatile uint32_t *)GPIOA_BSRR_ADDR; // GPIOA bit set/reset register

  volatile uint32_t *const gpioa_afrl_ptr =
      (volatile uint32_t *)GPIOA_AFRL_ADDR; // GPIOA AF register

  volatile uint32_t *const usart2_brr_ptr =
      (volatile uint32_t *)USART2_BRR_ADDR; // USART2 BRR register

  volatile uint32_t *const usart2_cr1_ptr =
      (volatile uint32_t *)USART2_CR1_ADDR; // USART2 CR1 register

  volatile uint32_t *const usart2_sr_ptr =
      (volatile uint32_t *)USART2_SR_ADDR; // USART2 SR register

  volatile uint32_t *const usart2_dr_ptr =
      (volatile uint32_t *)USART2_DR_ADDR; // USART2 DR register

  /* 1. Enable clock for GPIOA */
  *rcc_ahb1enr_ptr |= RCC_AHB1ENR_GPIOAEN_MASK;
  (void)*rcc_ahb1enr_ptr; // Dummy read to ensure the write is completed

  /* 2. Enable clock for USART2 */
  *rcc_apb1enr_ptr |= RCC_APB1ENR_USART2EN_MASK;
  (void)*rcc_apb1enr_ptr; // Dummy read to ensure the write is completed

  /* 3. Configure AFRL PA2 and PA3 as AF */
  /* Select AF7 for PA2 and PA3 */
  *gpioa_afrl_ptr =
      (*gpioa_afrl_ptr & ~GPIOA_AFRL_2_3_MASK) | GPIOA_AFRL_2_3_AF;

  /* 4. Configure PA2 and PA3 as AF and PA5 as general-purpose output */
  /* Clear mode bits for pin 2, 3 and 5 and set mode to AF and output */
  *gpioa_moder_ptr = (*gpioa_moder_ptr & ~(GPIOA_MODER_MODER_2_3_MASK |
                                           GPIOA_MODER_MODER_5_MASK)) |
                     (GPIOA_MODER_MODER_2_3_AF | GPIOA_MODER_MODER_5_OUTPUT);

  /* 5. Writing the calculated baud rate divisor to the BRR register */
  *usart2_brr_ptr = USART2_BRR_16_MHZ_115200_BAUD_VALUE;

  /* 6. Enable USART2 (UE) and the transmitter (TE) */
  *usart2_cr1_ptr |= (USART2_CR1_UE | USART2_CR1_TE);

  /* 7. Turn LED on initially (PA5 high) */
  *gpioa_bsrr_ptr = GPIOA_BSRR_BS5;

  int led_state = 1;
  volatile uint32_t blink_counter = 0;

  /* Main loop - software delay based blinking */
  while (1) {
    if (blink_counter >= 1000000u) {
      if (led_state == 1) {
        /* Turn LED off (PA5 low) */
        *gpioa_bsrr_ptr = GPIOA_BSRR_BR5;

        led_state = 0;
      } else if (led_state == 0) {
        /* Turn LED on (PA5 high) */
        *gpioa_bsrr_ptr = GPIOA_BSRR_BS5;

        led_state = 1;
      }

      /* Sending the string "Hello from STM32!\r\n" via UART */
      const char *msg = "Hello from STM32!\r\n";

      while (*msg != '\0') {
        // 1. Waiting for the transmit buffer to become free
        while ((*usart2_sr_ptr & USART2_TXE) == 0) {
        }

        // Writing the current character of the string into the data register
        *usart2_dr_ptr = (uint32_t)(*msg);

        // Moving on to the next symbol
        msg++;
      }

      // 2. Waiting for the transmission of the last byte
      while ((*usart2_sr_ptr & USART2_TC) == 0) {
      }

      blink_counter = 0;
    }

    blink_counter++;
  }
}

/* -------------------------------------------------------------------------- */
/* Vector table                                                               */
/* Must be placed at the beginning of Flash (usually address 0x08000000)      */
/* -------------------------------------------------------------------------- */
const uintptr_t VECTOR_TABLE[102] __attribute__((section(".vector_table"))) = {
    [0] = (uintptr_t)&STACK_TOP,               // Initial Stack Pointer
    [1] = (uintptr_t)&Reset_Handler,           // Reset Handler
    [2 ... 6] = (uintptr_t)&Default_Handler,   // NMI, HardFault, MemManage,
                                               // BusFault, UsageFault
    [11] = (uintptr_t)&Default_Handler,        // SVCall
    [12] = (uintptr_t)&Default_Handler,        // Debug Monitor
    [14 ... 15] = (uintptr_t)&Default_Handler, // PendSV, SysTick

    /* External interrupts (IRQ 0 and above) */
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