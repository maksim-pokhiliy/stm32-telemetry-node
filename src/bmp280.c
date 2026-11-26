#include "bmp280.h"
#include "i2c.h"
#include "status.h"
#include "systick.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BMP280_ADDR 0x76u

#define BMP280_REG_CALIB 0x88u
#define BMP280_REG_ID 0xD0u
#define BMP280_REG_RESET 0xE0u
#define BMP280_REG_STATUS 0xF3u
#define BMP280_REG_CTRL_MEAS 0xF4u
#define BMP280_REG_CONFIG 0xF5u

#define BMP280_ID_VALUE 0x58u
#define BMP280_RESET_CMD 0xB6u
#define BMP280_STATUS_IM_UPDATE (1u << 0)

#define BMP280_CONFIG_VALUE 0x28u
#define BMP280_CTRL_MEAS_VALUE 0x57u

#define BMP280_CALIB_LEN 24u
#define BMP280_RAW_LEN 6u
#define BMP280_STARTUP_DELAY_MS 2u
#define BMP280_IM_UPDATE_TIMEOUT_MS 5u
#define BMP280_REG_PRESS_MSB 0xF7u

_Static_assert((int16_t)(uint16_t)0x8000u == -32768,
               "uint16_t -> int16_t must wrap modulo 2^16");

_Static_assert((int16_t)(uint16_t)0xF220u == -3552,
               "uint16_t -> int16_t must wrap modulo 2^16");

static bmp280_calib_t bmp280_calib;
static bool bmp280_calib_valid = false;

static uint16_t bmp280_make_uint16_le(uint8_t lsb, uint8_t msb);
static int16_t bmp280_make_int16_le(uint8_t lsb, uint8_t msb);
static uint32_t bmp280_make_uint20(uint8_t msb, uint8_t lsb, uint8_t xlsb);
static status_t bmp280_wait_im_update(void);
static void bmp280_decode_calib(const uint8_t *raw);

static uint16_t bmp280_make_uint16_le(uint8_t lsb, uint8_t msb) {
  return (uint16_t)(((uint16_t)msb << 8) | lsb);
}

static int16_t bmp280_make_int16_le(uint8_t lsb, uint8_t msb) {
  return (int16_t)bmp280_make_uint16_le(lsb, msb);
}

static uint32_t bmp280_make_uint20(uint8_t msb, uint8_t lsb, uint8_t xlsb) {
  return ((uint32_t)msb << 12) | ((uint32_t)lsb << 4) | ((uint32_t)xlsb >> 4);
}

static void bmp280_decode_calib(const uint8_t *raw) {
  bmp280_calib.dig_T1 = bmp280_make_uint16_le(raw[0], raw[1]);
  bmp280_calib.dig_T2 = bmp280_make_int16_le(raw[2], raw[3]);
  bmp280_calib.dig_T3 = bmp280_make_int16_le(raw[4], raw[5]);
  bmp280_calib.dig_P1 = bmp280_make_uint16_le(raw[6], raw[7]);
  bmp280_calib.dig_P2 = bmp280_make_int16_le(raw[8], raw[9]);
  bmp280_calib.dig_P3 = bmp280_make_int16_le(raw[10], raw[11]);
  bmp280_calib.dig_P4 = bmp280_make_int16_le(raw[12], raw[13]);
  bmp280_calib.dig_P5 = bmp280_make_int16_le(raw[14], raw[15]);
  bmp280_calib.dig_P6 = bmp280_make_int16_le(raw[16], raw[17]);
  bmp280_calib.dig_P7 = bmp280_make_int16_le(raw[18], raw[19]);
  bmp280_calib.dig_P8 = bmp280_make_int16_le(raw[20], raw[21]);
  bmp280_calib.dig_P9 = bmp280_make_int16_le(raw[22], raw[23]);
}

static status_t bmp280_wait_im_update(void) {
  uint32_t start_ms = systick_get_ms();

  while (1) {
    uint8_t status_reg = 0u;

    status_t status =
        i2c_read_reg(BMP280_ADDR, BMP280_REG_STATUS, &status_reg, 1);

    if (status == DRIVER_OK) {
      if ((status_reg & BMP280_STATUS_IM_UPDATE) == 0u) {
        return DRIVER_OK;
      }
    } else if (status != DRIVER_NO_ACK_ADDR) {
      return status;
    }

    if (systick_elapsed_ms(systick_get_ms(), start_ms) >=
        BMP280_IM_UPDATE_TIMEOUT_MS) {
      return DRIVER_TIMEOUT;
    }
  }
}

status_t bmp280_init(void) {
  bmp280_calib_valid = false;

  uint8_t id = 0u;

  status_t status = i2c_read_reg(BMP280_ADDR, BMP280_REG_ID, &id, 1);

  if (status != DRIVER_OK) {
    return status;
  }

  if (id != BMP280_ID_VALUE) {
    return DRIVER_WRONG_ID;
  }

  status = i2c_write_reg(BMP280_ADDR, BMP280_REG_RESET, BMP280_RESET_CMD);

  if (status != DRIVER_OK) {
    return status;
  }

  systick_delay_ms(BMP280_STARTUP_DELAY_MS);

  status = bmp280_wait_im_update();

  if (status != DRIVER_OK) {
    return status;
  }

  uint8_t calib_raw[BMP280_CALIB_LEN];

  status =
      i2c_read_reg(BMP280_ADDR, BMP280_REG_CALIB, calib_raw, BMP280_CALIB_LEN);

  if (status != DRIVER_OK) {
    return status;
  }

  bmp280_decode_calib(calib_raw);

  status = i2c_write_reg(BMP280_ADDR, BMP280_REG_CONFIG, BMP280_CONFIG_VALUE);

  if (status != DRIVER_OK) {
    return status;
  }

  status =
      i2c_write_reg(BMP280_ADDR, BMP280_REG_CTRL_MEAS, BMP280_CTRL_MEAS_VALUE);

  if (status != DRIVER_OK) {
    return status;
  }

  bmp280_calib_valid = true;

  return DRIVER_OK;
}

const bmp280_calib_t *bmp280_get_calibration(void) {
  if (!bmp280_calib_valid) {
    return NULL;
  }

  return &bmp280_calib;
}

status_t bmp280_read_raw(bmp280_raw_sample_t *sample) {
  if (sample == NULL) {
    return DRIVER_INVALID_ARG;
  }

  uint8_t raw[BMP280_RAW_LEN];

  status_t status =
      i2c_read_reg(BMP280_ADDR, BMP280_REG_PRESS_MSB, raw, BMP280_RAW_LEN);

  if (status != DRIVER_OK) {
    return status;
  }

  sample->raw_press = bmp280_make_uint20(raw[0], raw[1], raw[2]);
  sample->raw_temp = bmp280_make_uint20(raw[3], raw[4], raw[5]);

  return DRIVER_OK;
}
