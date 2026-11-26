#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

void systick_init(void);
uint32_t systick_get_ms(void);
uint32_t systick_elapsed_ms(uint32_t now_ms, uint32_t start_ms);
void systick_delay_ms(uint32_t duration_ms);

#endif
