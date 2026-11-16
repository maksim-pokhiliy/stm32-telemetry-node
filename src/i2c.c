#include "i2c.h"
#include "status.h"
#include "stm32f411_regs.h"
#include <stdint.h>

#define I2C_FREQ_16MHZ 16u
#define I2C_CCR_100KHZ 80u
#define I2C_TRISE_16MHZ 17u

#define I2C_TIMEOUT_MS 5u

extern volatile uint32_t systick_ms;

static status_t i2c_wait_flag(volatile uint32_t *reg, uint32_t mask,
                              bool wait_set);

static status_t i2c_wait_address_result(volatile uint32_t *sr1);
static bool i2c_lines_high(void);
static status_t i2c_wait_lines_high(void);
static status_t i2c_recover(void);
static void i2c_reset_peripheral(void);

status_t i2c_init(void) {
  volatile uint32_t *const rcc_ahb1enr = (volatile uint32_t *)RCC_AHB1ENR_ADDR;
  volatile uint32_t *const rcc_apb1enr = (volatile uint32_t *)RCC_APB1ENR_ADDR;
  volatile uint32_t *const i2c_cr1 = (volatile uint32_t *)I2C1_CR1_ADDR;
  volatile uint32_t *const i2c_cr2 = (volatile uint32_t *)I2C1_CR2_ADDR;
  volatile uint32_t *const i2c_oar1 = (volatile uint32_t *)I2C1_OAR1_ADDR;
  volatile uint32_t *const i2c_ccr = (volatile uint32_t *)I2C1_CCR_ADDR;
  volatile uint32_t *const i2c_trise = (volatile uint32_t *)I2C1_TRISE_ADDR;

  volatile uint32_t *const gpiob_otyper =
      (volatile uint32_t *)GPIOB_OTYPER_ADDR;

  volatile uint32_t *const gpiob_pupdr = (volatile uint32_t *)GPIOB_PUPDR_ADDR;

  volatile uint32_t *const gpiob_ospeedr =
      (volatile uint32_t *)GPIOB_OSPEEDR_ADDR;

  volatile uint32_t *const gpiob_afrh = (volatile uint32_t *)GPIOB_AFRH_ADDR;
  volatile uint32_t *const gpiob_moder = (volatile uint32_t *)GPIOB_MODER_ADDR;

  const uint32_t pins_otyper_mask =
      GPIOB_OTYPER_PB8_MASK | GPIOB_OTYPER_PB9_MASK;

  const uint32_t pins_pupdr_mask = GPIOB_PUPDR_PB8_MASK | GPIOB_PUPDR_PB9_MASK;

  const uint32_t pins_ospeedr_mask =
      GPIOB_OSPEEDR_PB8_MASK | GPIOB_OSPEEDR_PB9_MASK;

  const uint32_t pins_afrh_mask = GPIOB_AFRH_PB8_MASK | GPIOB_AFRH_PB9_MASK;
  const uint32_t pins_moder_mask = GPIOB_MODER_PB8_MASK | GPIOB_MODER_PB9_MASK;

  const uint32_t pins_otyper_value = GPIOB_OTYPER_PB8_OD | GPIOB_OTYPER_PB9_OD;

  const uint32_t pins_pupdr_value =
      GPIOB_PUPDR_PB8_NOPULL | GPIOB_PUPDR_PB9_NOPULL;

  const uint32_t pins_ospeedr_value =
      GPIOB_OSPEEDR_PB8_MEDIUM | GPIOB_OSPEEDR_PB9_MEDIUM;

  const uint32_t pins_afrh_value = GPIOB_AFRH_PB8_AF4 | GPIOB_AFRH_PB9_AF4;
  const uint32_t pins_moder_value = GPIOB_MODER_PB8_AF | GPIOB_MODER_PB9_AF;

  *rcc_ahb1enr |= RCC_AHB1ENR_GPIOBEN_MASK;
  (void)*rcc_ahb1enr;

  *rcc_apb1enr |= RCC_APB1ENR_I2C1EN_MASK;
  (void)*rcc_apb1enr;

  *i2c_cr1 &= ~I2C_CR1_PE_MASK;

  *gpiob_otyper = (*gpiob_otyper & ~pins_otyper_mask) | pins_otyper_value;
  *gpiob_pupdr = (*gpiob_pupdr & ~pins_pupdr_mask) | pins_pupdr_value;
  *gpiob_ospeedr = (*gpiob_ospeedr & ~pins_ospeedr_mask) | pins_ospeedr_value;
  *gpiob_afrh = (*gpiob_afrh & ~pins_afrh_mask) | pins_afrh_value;
  *gpiob_moder = (*gpiob_moder & ~pins_moder_mask) | pins_moder_value;

  *i2c_cr2 =
      (*i2c_cr2 & ~I2C_CR2_FREQ_MASK) | (I2C_FREQ_16MHZ & I2C_CR2_FREQ_MASK);

  *i2c_ccr = (*i2c_ccr & ~(I2C_CCR_CCR_MASK | I2C_CCR_FS_MASK)) |
             (I2C_CCR_100KHZ & I2C_CCR_CCR_MASK);

  *i2c_trise = (*i2c_trise & ~I2C_TRISE_TRISE_MASK) |
               (I2C_TRISE_16MHZ & I2C_TRISE_TRISE_MASK);

  *i2c_oar1 |= I2C_OAR1_BIT14_MASK;
  *i2c_cr1 |= I2C_CR1_PE_MASK;

  return DRIVER_OK;
}

static status_t i2c_wait_flag(volatile uint32_t *reg, uint32_t mask,
                              bool wait_set) {
  uint32_t start_ms = systick_ms;

  while (1) {
    bool bit_is_set = (*reg & mask) != 0;

    if (bit_is_set == wait_set) {
      return DRIVER_OK;
    }

    if ((systick_ms - start_ms) >= I2C_TIMEOUT_MS) {
      return DRIVER_TIMEOUT;
    }
  }
}

static status_t i2c_wait_address_result(volatile uint32_t *sr1) {
  uint32_t start_ms = systick_ms;

  while (1) {
    uint32_t sr1_val = *sr1;

    bool af_is_set = (sr1_val & I2C_SR1_AF_MASK) != 0;
    bool addr_is_set = (sr1_val & I2C_SR1_ADDR_MASK) != 0;

    if (af_is_set) {
      return DRIVER_NO_ACK;
    }

    if (addr_is_set) {
      return DRIVER_OK;
    }

    if ((systick_ms - start_ms) >= I2C_TIMEOUT_MS) {
      return DRIVER_TIMEOUT;
    }
  }
}

static bool i2c_lines_high(void) {
  volatile uint32_t *const gpiob_idr = (volatile uint32_t *)GPIOB_IDR_ADDR;

  uint32_t idr = *gpiob_idr;

  bool pb8_is_high = (idr & GPIOB_IDR_PB8_MASK) != 0;
  bool pb9_is_high = (idr & GPIOB_IDR_PB9_MASK) != 0;

  return pb8_is_high && pb9_is_high;
}

static status_t i2c_wait_lines_high(void) {
  uint32_t start_ms = systick_ms;

  while (1) {
    bool i2c_is_lines_high = i2c_lines_high();

    if (i2c_is_lines_high) {
      return DRIVER_OK;
    }

    if ((systick_ms - start_ms) >= I2C_TIMEOUT_MS) {
      return DRIVER_TIMEOUT;
    }
  }
}

static void i2c_reset_peripheral(void) {
  volatile uint32_t *const rcc_apb1rstr =
      (volatile uint32_t *)RCC_APB1RSTR_ADDR;

  *rcc_apb1rstr |= RCC_APB1RSTR_I2C1RST_MASK;
  (void)*rcc_apb1rstr;

  *rcc_apb1rstr &= ~RCC_APB1RSTR_I2C1RST_MASK;
  (void)*rcc_apb1rstr;
}

static status_t check_status(status_t status) {
  if (status == DRIVER_TIMEOUT) {
    i2c_recover();
  }

  return status;
}

static status_t i2c_recover(void) {
  i2c_reset_peripheral();

  volatile uint32_t *const i2c1_sr2 = (volatile uint32_t *)I2C1_SR2_ADDR;

  status_t lines_status = i2c_wait_lines_high();

  if (lines_status != DRIVER_OK) {
    return lines_status;
  }

  status_t init_status = i2c_init();

  if (init_status != DRIVER_OK) {
    i2c_reset_peripheral();

    return init_status;
  }

  status_t busy_status = i2c_wait_flag(i2c1_sr2, I2C_SR2_BUSY_MASK, false);

  if (busy_status != DRIVER_OK) {
    i2c_reset_peripheral();
  }

  return busy_status;
}

status_t i2c_probe(uint8_t address) {
  volatile uint32_t *const i2c1_cr1 = (volatile uint32_t *)I2C1_CR1_ADDR;
  volatile uint32_t *const i2c1_sr1 = (volatile uint32_t *)I2C1_SR1_ADDR;
  volatile uint32_t *const i2c1_sr2 = (volatile uint32_t *)I2C1_SR2_ADDR;
  volatile uint32_t *const i2c1_dr = (volatile uint32_t *)I2C1_DR_ADDR;

  uint32_t cr1 = *i2c1_cr1;

  bool pe_is_set = (cr1 & I2C_CR1_PE_MASK) != 0;

  if (!pe_is_set) {
    status_t i2c_recover_status = i2c_recover();

    if (i2c_recover_status != DRIVER_OK) {
      return i2c_recover_status;
    }
  }

  status_t busy_status = i2c_wait_flag(i2c1_sr2, I2C_SR2_BUSY_MASK, false);

  if (check_status(busy_status) != DRIVER_OK) {
    return busy_status;
  }

  *i2c1_cr1 |= I2C_CR1_START_MASK;

  status_t sb_status = i2c_wait_flag(i2c1_sr1, I2C_SR1_SB_MASK, true);

  if (check_status(sb_status) != DRIVER_OK) {
    return sb_status;
  }

  *i2c1_dr = (uint8_t)(address << 1);

  status_t address_status = i2c_wait_address_result(i2c1_sr1);

  if (address_status == DRIVER_TIMEOUT) {
    i2c_recover();

    return address_status;
  }

  if (address_status == DRIVER_OK) {
    (void)*i2c1_sr1;
    (void)*i2c1_sr2;
  }

  if (address_status == DRIVER_NO_ACK) {
    *i2c1_sr1 &= ~I2C_SR1_AF_MASK;
  }

  *i2c1_cr1 |= I2C_CR1_STOP_MASK;

  status_t stop_status = i2c_wait_flag(i2c1_cr1, I2C_CR1_STOP_MASK, false);

  if (check_status(stop_status) != DRIVER_OK) {
    return stop_status;
  }

  status_t busy_cleared = i2c_wait_flag(i2c1_sr2, I2C_SR2_BUSY_MASK, false);

  if (check_status(busy_cleared) != DRIVER_OK) {
    return busy_cleared;
  }

  return address_status;
}
