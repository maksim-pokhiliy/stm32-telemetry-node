#include <stdint.h>

extern unsigned char STACK_TOP;

extern unsigned char DATA_FLASH_IMAGE_START;

extern unsigned char DATA_SRAM_START;
extern unsigned char DATA_SRAM_END;

extern unsigned char BSS_SRAM_START;
extern unsigned char BSS_SRAM_END;

uint32_t counter = 1;
uint32_t errors;

void Reset_Handler(void);
void Default_Handler(void);

int main(void);

void run_infinite_loop(void);

void Reset_Handler(void) {
  const uint32_t *data_src = (uint32_t *)&DATA_FLASH_IMAGE_START;

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
  run_infinite_loop();
}

void Default_Handler(void) {
  while (1) {
  }
}

int main(void) { run_infinite_loop(); }

void run_infinite_loop(void) {
  while (1) {
    counter++;
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
