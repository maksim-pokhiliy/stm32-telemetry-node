#include "systick.h"
#include "stm32f411_regs.h"

#define SYSTICK_RELOAD_16MHZ_1MS 15999u

static volatile uint32_t systick_ms;

void SysTick_Handler(void) { systick_ms++; }

void systick_init(void) {
  volatile uint32_t *const stk_ctrl = (volatile uint32_t *)STK_CTRL_ADDR;
  volatile uint32_t *const stk_load = (volatile uint32_t *)STK_LOAD_ADDR;
  volatile uint32_t *const stk_val = (volatile uint32_t *)STK_VAL_ADDR;

  *stk_ctrl = 0u;
  *stk_load = SYSTICK_RELOAD_16MHZ_1MS;
  *stk_val = 0u;

  *stk_ctrl =
      STK_CTRL_ENABLE_MASK | STK_CTRL_TICKINT_MASK | STK_CTRL_CLKSOURCE_MASK;
}

uint32_t systick_get_ms(void) { return systick_ms; }
