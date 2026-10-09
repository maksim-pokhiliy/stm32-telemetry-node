#include <stdint.h>

extern unsigned char STACK_TOP;

void Reset_Handler(void);

void Reset_Handler(void) {
  while (1) {
  }
}

const uintptr_t VECTOR_TABLE[2] __attribute__((section(".vector_table"))) = {
    (uintptr_t)&STACK_TOP, (uintptr_t)&Reset_Handler};
