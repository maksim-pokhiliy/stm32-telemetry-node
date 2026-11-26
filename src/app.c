#include "bmp280.h"
#include "conversions.h"
#include "i2c.h"
#include "mpu6050.h"
#include "selftest.h"
#include "status.h"
#include "stm32f411_regs.h"
#include "systick.h"
#include "uart.h"
#include <stdbool.h>
#include <stddef.h>
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
static void bmp280_print_calib(const bmp280_calib_t *calib);
static void bmp280_calib_double_read_diag(void);
static void bmp280_raw_after_reset_diag(void);
static void bmp280_stream_step(void);
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
  uart_send_decimal(" bus_clear=", i2c_get_bus_clear_count());
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

static void bmp280_print_calib(const bmp280_calib_t *calib) {
  uart_send_message("dig_T1=");
  uart_send_uint32(calib->dig_T1);
  uart_send_message("\r\n");

  uart_send_message("dig_T2=");
  uart_send_int32(calib->dig_T2);
  uart_send_message("\r\n");

  uart_send_message("dig_T3=");
  uart_send_int32(calib->dig_T3);
  uart_send_message("\r\n");

  uart_send_message("dig_P1=");
  uart_send_uint32(calib->dig_P1);
  uart_send_message("\r\n");

  uart_send_message("dig_P2=");
  uart_send_int32(calib->dig_P2);
  uart_send_message("\r\n");

  uart_send_message("dig_P3=");
  uart_send_int32(calib->dig_P3);
  uart_send_message("\r\n");

  uart_send_message("dig_P4=");
  uart_send_int32(calib->dig_P4);
  uart_send_message("\r\n");

  uart_send_message("dig_P5=");
  uart_send_int32(calib->dig_P5);
  uart_send_message("\r\n");

  uart_send_message("dig_P6=");
  uart_send_int32(calib->dig_P6);
  uart_send_message("\r\n");

  uart_send_message("dig_P7=");
  uart_send_int32(calib->dig_P7);
  uart_send_message("\r\n");

  uart_send_message("dig_P8=");
  uart_send_int32(calib->dig_P8);
  uart_send_message("\r\n");

  uart_send_message("dig_P9=");
  uart_send_int32(calib->dig_P9);
  uart_send_message("\r\n");
}

static void bmp280_calib_double_read_diag(void) {
  (void)i2c_write_reg(0x76u, 0xE0u, 0xB6u);
  systick_delay_ms(2u);

  uint8_t first[24];
  uint8_t second[24];

  status_t st1 = i2c_read_reg(0x76u, 0x88u, first, 24);
  status_t st2 = i2c_read_reg(0x76u, 0x88u, second, 24);

  bool match = true;

  if (st1 == DRIVER_OK && st2 == DRIVER_OK) {
    for (size_t i = 0u; i < 24u; i++) {
      if (first[i] != second[i]) {
        match = false;
        break;
      }
    }
  } else {
    match = false;
  }

  uart_send_value("bmp280 calib double-read st1=0x", (uint32_t)st1);
  uart_send_value(" st2=0x", (uint32_t)st2);
  uart_send_value(" match=0x", match ? 1u : 0u);
  uart_send_message("\r\n");
}

static void bmp280_raw_after_reset_diag(void) {
  (void)i2c_write_reg(0x76u, 0xE0u, 0xB6u);
  systick_delay_ms(2u);

  uint8_t raw[6] = {0};

  status_t status = i2c_read_reg(0x76u, 0xF7u, raw, 6);

  uart_send_value("bmp280 raw after reset status=0x", (uint32_t)status);
  uart_send_message("\r\n");

  if (status == DRIVER_OK) {
    uart_send_value("F7=0x", raw[0]);
    uart_send_value(" F8=0x", raw[1]);
    uart_send_value(" F9=0x", raw[2]);
    uart_send_value(" FA=0x", raw[3]);
    uart_send_value(" FB=0x", raw[4]);
    uart_send_value(" FC=0x", raw[5]);
    uart_send_message("\r\n");

    uint32_t raw_press = ((uint32_t)raw[0] << 12) | ((uint32_t)raw[1] << 4) |
                         ((uint32_t)raw[2] >> 4);
    uint32_t raw_temp = ((uint32_t)raw[3] << 12) | ((uint32_t)raw[4] << 4) |
                        ((uint32_t)raw[5] >> 4);

    uart_send_decimal("raw_press=", raw_press);
    uart_send_message("\r\n");
    uart_send_decimal("raw_temp=", raw_temp);
    uart_send_message("\r\n");
  }
}

static void bmp280_stream_step(void) {
  bmp280_raw_sample_t sample;

  status_t status = bmp280_read_raw(&sample);

  if (status == DRIVER_OK) {
    uart_send_decimal("bmp raw_press=", sample.raw_press);
    uart_send_decimal(" raw_temp=", sample.raw_temp);
    uart_send_message("\r\n");
  } else {
    uart_send_value("bmp read: status=0x", (uint32_t)status);
    uart_send_message("\r\n");
  }
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
  bool bmp_is_ready = false;

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

    bmp280_calib_double_read_diag();
    bmp280_raw_after_reset_diag();

    status_t bmp_init_result = bmp280_init();

    uart_send_value("bmp280_init: status=0x", (uint32_t)bmp_init_result);
    uart_send_message("\r\n");

    if (bmp_init_result == DRIVER_OK) {
      const bmp280_calib_t *calib = bmp280_get_calibration();

      if (calib != NULL) {
        bmp280_print_calib(calib);
      }

      uint8_t config = 0u;
      uint8_t ctrl_meas = 0u;

      status_t config_status = i2c_read_reg(0x76u, 0xF5u, &config, 1);
      status_t ctrl_status = i2c_read_reg(0x76u, 0xF4u, &ctrl_meas, 1);

      if (config_status == DRIVER_OK) {
        uart_send_value("bmp280 config=0x", config);
        uart_send_message("\r\n");
      } else {
        uart_send_value("bmp280 config read status=0x",
                        (uint32_t)config_status);
        uart_send_message("\r\n");
      }

      if (ctrl_status == DRIVER_OK) {
        uart_send_value("bmp280 ctrl_meas=0x", ctrl_meas);
        uart_send_message("\r\n");
      } else {
        uart_send_value("bmp280 ctrl_meas read status=0x",
                        (uint32_t)ctrl_status);
        uart_send_message("\r\n");
      }

      bmp_is_ready = true;
    }
  } else {
    uart_send_message("i2c_init: FAIL\r\n");
  }

  bool led_is_on = true;
  uint32_t last_led_toggle_ms = systick_get_ms();
  uint32_t last_sample_ms = systick_get_ms();
  uint32_t last_bmp_sample_ms = systick_get_ms();
  uint32_t last_uptime_report_ms = systick_get_ms();

  while (1) {
    uint32_t now_ms = systick_get_ms();

    if (systick_elapsed_ms(now_ms, last_led_toggle_ms) >= LED_TOGGLE_DELAY_MS) {
      if (led_is_on) {
        *gpioa_bsrr = GPIOA_BSRR_BR5;
      } else {
        *gpioa_bsrr = GPIOA_BSRR_BS5;
      }

      led_is_on = !led_is_on;
      last_led_toggle_ms = now_ms;
    }

    if (mpu_is_ready &&
        (systick_elapsed_ms(now_ms, last_sample_ms) >= MPU_SAMPLE_PERIOD_MS)) {
      mpu_stream_step();

      last_sample_ms = now_ms;
    }

    if (bmp_is_ready && (systick_elapsed_ms(now_ms, last_bmp_sample_ms) >=
                         MPU_SAMPLE_PERIOD_MS)) {
      bmp280_stream_step();

      last_bmp_sample_ms = now_ms;
    }

    if (systick_elapsed_ms(now_ms, last_uptime_report_ms) >=
        UART_REPORT_DELAY_MS) {
      uptime_report(now_ms);

      last_uptime_report_ms = now_ms;
    }
  }
}
