#include "conversions.h"
#include "i2c.h"
#include "mpu6050.h"
#include "selftest.h"
#include "status.h"
#include "stm32f411_regs.h"
#include "systick.h"
#include "uart.h"
#include <stdbool.h>
#include <stdint.h>

#define LED_TOGGLE_DELAY_MS 500u
#define UART_REPORT_DELAY_MS 1000u

#define MPU_SAMPLE_PERIOD_MS 100u
#define MPU_SAMPLE_VALUE_COUNT 7u

static mpu6050_sample_t mpu_sample;
static uint32_t mpu_ok_count;
static uint32_t mpu_nack_count;
static uint32_t mpu_timeout_count;
static uint32_t mpu_bus_error_count;
static uint32_t mpu_other_count;

static void uptime_report(uint32_t now_ms);
static void mpu_print_sample(const mpu6050_sample_t *sample);
static void mpu_stream_step(void);
static void mpu_print_temp_centi(int16_t raw);
static void user_button_init(void);
static bool user_button_is_pressed(void);

static void mpu_print_sample(const mpu6050_sample_t *sample) {
  const int16_t values[MPU_SAMPLE_VALUE_COUNT] = {
      sample->accel_x, sample->accel_y, sample->accel_z, sample->temp,
      sample->gyro_x,  sample->gyro_y,  sample->gyro_z,
  };

  for (uint32_t i = 0u; i < MPU_SAMPLE_VALUE_COUNT; i++) {
    if (i > 0u) {
      uart_send_byte(' ');
    }

    uart_send_int32(values[i]);
  }

  uart_send_message("\r\n");
}

static void mpu_stream_step(void) {
  status_t status = mpu6050_read_sample(&mpu_sample);

  if (status == DRIVER_OK) {
    static bool first_sample_reported = false;

    if (!first_sample_reported) {
      uart_send_decimal("boot_first_sample_ms=", systick_get_ms());
      uart_send_message("\r\n");

      first_sample_reported = true;
    }

    mpu_ok_count++;

    mpu_print_sample(&mpu_sample);
    mpu_print_temp_centi(mpu_sample.temp);
  } else {
    if (status == DRIVER_NO_ACK_ADDR || status == DRIVER_NO_ACK_DATA) {
      mpu_nack_count++;
    } else if (status == DRIVER_TIMEOUT) {
      mpu_timeout_count++;
    } else if (status == DRIVER_BUS_ERROR) {
      mpu_bus_error_count++;
    } else {
      mpu_other_count++;
    }

    uart_send_value("mpu read: status=0x", (uint32_t)status);
    uart_send_message("\r\n");
  }
}

static void mpu_print_temp_centi(int16_t raw) {
  uart_send_message("temp_centi=");
  uart_send_int32(mpu_temp_raw_to_centi(raw));
  uart_send_message("\r\n");
}

static void uptime_report(uint32_t now_ms) {
  uart_send_decimal("uptime_s=", now_ms / 1000u);
  uart_send_decimal(" ok=", mpu_ok_count);
  uart_send_decimal(" nack=", mpu_nack_count);
  uart_send_decimal(" timeout=", mpu_timeout_count);
  uart_send_decimal(" bus=", mpu_bus_error_count);
  uart_send_decimal(" other=", mpu_other_count);
  uart_send_decimal(" recover=", i2c_get_recover_count());
  uart_send_message("\r\n");

  if (mpu_ok_count > 0u) {
    uart_send_message("  last: ");
    mpu_print_sample(&mpu_sample);
  }

  mpu_ok_count = 0u;
  mpu_nack_count = 0u;
  mpu_timeout_count = 0u;
  mpu_bus_error_count = 0u;
  mpu_other_count = 0u;
}

static void user_button_init(void) {
  volatile uint32_t *const rcc_ahb1enr = (volatile uint32_t *)RCC_AHB1ENR_ADDR;
  volatile uint32_t *const gpioc_moder = (volatile uint32_t *)GPIOC_MODER_ADDR;
  volatile uint32_t *const gpioc_pupdr = (volatile uint32_t *)GPIOC_PUPDR_ADDR;

  *rcc_ahb1enr |= RCC_AHB1ENR_GPIOCEN_MASK;
  (void)*rcc_ahb1enr;

  *gpioc_moder =
      (*gpioc_moder & ~GPIOC_MODER_PC13_MASK) | GPIOC_MODER_PC13_INPUT;

  *gpioc_pupdr =
      (*gpioc_pupdr & ~GPIOC_PUPDR_PC13_MASK) | GPIOC_PUPDR_PC13_NOPULL;
}

static bool user_button_is_pressed(void) {
  volatile uint32_t *const gpioc_idr = (volatile uint32_t *)GPIOC_IDR_ADDR;

  return (*gpioc_idr & GPIOC_IDR_PC13_MASK) == 0u;
}

int main(void) {
  volatile uint32_t *const gpioa_moder = (volatile uint32_t *)GPIOA_MODER_ADDR;
  volatile uint32_t *const gpioa_bsrr = (volatile uint32_t *)GPIOA_BSRR_ADDR;
  volatile uint32_t *const rcc_ahb1enr = (volatile uint32_t *)RCC_AHB1ENR_ADDR;

  const uint32_t pin_5_mode_mask = GPIOA_MODER_PA5_MASK;
  const uint32_t pin_5_mode_value = GPIOA_MODER_PA5_OUTPUT;

  *rcc_ahb1enr |= RCC_AHB1ENR_GPIOAEN_MASK;
  (void)*rcc_ahb1enr;

  *gpioa_moder = (*gpioa_moder & ~pin_5_mode_mask) | pin_5_mode_value;
  *gpioa_bsrr = GPIOA_BSRR_BS5;

  systick_init();

  uart_init();
  user_button_init();

  bool selftest_requested = user_button_is_pressed();

  if (selftest_requested) {
    uart_send_message("boot mode: SELFTEST\r\n");
  } else {
    uart_send_message("boot mode: NORMAL\r\n");
  }

  bool mpu_is_ready = false;

  status_t i2c_init_result = i2c_init();

  if (i2c_init_result == DRIVER_OK) {
    uart_send_message("i2c_init: OK\r\n");

    if (selftest_requested) {
      selftest_run();
    }

    status_t mpu_init_result = mpu6050_init();

    uart_send_value("mpu6050_init: status=0x", (uint32_t)mpu_init_result);
    uart_send_message("\r\n");

    mpu_is_ready = (mpu_init_result == DRIVER_OK);
  } else {
    uart_send_message("i2c_init: FAIL\r\n");
  }

  bool led_is_on = true;
  uint32_t last_led_toggle_ms = systick_get_ms();
  uint32_t last_sample_ms = systick_get_ms();
  uint32_t last_uptime_report_ms = systick_get_ms();

  while (1) {
    uint32_t now_ms = systick_get_ms();

    if (now_ms - last_led_toggle_ms >= LED_TOGGLE_DELAY_MS) {
      if (led_is_on) {
        *gpioa_bsrr = GPIOA_BSRR_BR5;
      } else {
        *gpioa_bsrr = GPIOA_BSRR_BS5;
      }

      led_is_on = !led_is_on;
      last_led_toggle_ms = now_ms;
    }

    if (mpu_is_ready && (now_ms - last_sample_ms >= MPU_SAMPLE_PERIOD_MS)) {
      mpu_stream_step();

      last_sample_ms = now_ms;
    }

    if (now_ms - last_uptime_report_ms >= UART_REPORT_DELAY_MS) {
      uptime_report(now_ms);

      last_uptime_report_ms = now_ms;
    }
  }
}
