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
_Static_assert((-8 >> 1) == -4, "signed right shift must be arithmetic");
_Static_assert((int32_t)0x80000000u >> 1 == (int32_t)0xC0000000u,
               "signed right shift of INT32_MIN must be arithmetic");
_Static_assert((int64_t)-8 >> 1 == (int64_t)-4,
               "signed right shift of int64_t must be arithmetic");
_Static_assert((int64_t)0x8000000000000000u >> 1 ==
                   (int64_t)0xC000000000000000u,
               "signed right shift of INT64_MIN must be arithmetic");

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

status_t bmp280_compensate_temp(uint32_t raw_temp, const bmp280_calib_t *calib,
                                int32_t *temp_centi, int32_t *t_fine) {
  if (calib == NULL || temp_centi == NULL || t_fine == NULL) {
    return DRIVER_INVALID_ARG;
  }

  if (raw_temp > 0xFFFFFu) {
    return DRIVER_INVALID_ARG;
  }

  const int32_t adc_T = (int32_t)raw_temp;
  const int32_t dig_T1 = (int32_t)calib->dig_T1;
  const int32_t dig_T2 = (int32_t)calib->dig_T2;
  const int32_t dig_T3 = (int32_t)calib->dig_T3;

  const int32_t var1 = ((((adc_T >> 3) - (dig_T1 << 1)) * dig_T2) >> 11);

  const int32_t var2 =
      ((((((adc_T >> 4) - dig_T1) * ((adc_T >> 4) - dig_T1)) >> 12) * dig_T3) >>
       14);

  const int32_t fine = var1 + var2;

  *t_fine = fine;
  *temp_centi = (fine * 5 + 128) >> 8;

  return DRIVER_OK;
}

status_t bmp280_compensate_press(uint32_t raw_press,
                                 const bmp280_calib_t *calib, int32_t t_fine,
                                 uint32_t *press_q24_8) {
  if (calib == NULL || press_q24_8 == NULL) {
    return DRIVER_INVALID_ARG;
  }

  if (raw_press > 0xFFFFFu) {
    return DRIVER_INVALID_ARG;
  }

  const int64_t adc_P = (int64_t)raw_press;
  const int64_t dig_P1 = (int64_t)calib->dig_P1;
  const int64_t dig_P2 = (int64_t)calib->dig_P2;
  const int64_t dig_P3 = (int64_t)calib->dig_P3;
  const int64_t dig_P4 = (int64_t)calib->dig_P4;
  const int64_t dig_P5 = (int64_t)calib->dig_P5;
  const int64_t dig_P6 = (int64_t)calib->dig_P6;
  const int64_t dig_P7 = (int64_t)calib->dig_P7;
  const int64_t dig_P8 = (int64_t)calib->dig_P8;
  const int64_t dig_P9 = (int64_t)calib->dig_P9;

  int64_t var1 = (int64_t)t_fine - 128000;
  int64_t var2 = var1 * var1 * dig_P6;
  var2 = var2 + (var1 * dig_P5 * 131072);
  var2 = var2 + (dig_P4 * 34359738368);

  var1 = ((var1 * var1 * dig_P3) >> 8) + (var1 * dig_P2 * 4096);
  var1 = ((((int64_t)1 << 47) + var1) * dig_P1) >> 33;

  if (var1 == 0) {
    return DRIVER_INVALID_ARG;
  }

  int64_t p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;

  var1 = ((dig_P9 * (p >> 13) * (p >> 13)) >> 25);
  var2 = ((dig_P8 * p) >> 19);
  p = ((p + var1 + var2) >> 8) + (dig_P7 * 16);

  *press_q24_8 = (uint32_t)p;

  return DRIVER_OK;
}

status_t bmp280_compensate(const bmp280_raw_sample_t *raw,
                           const bmp280_calib_t *calib,
                           bmp280_sample_t *sample) {
  if (raw == NULL || calib == NULL || sample == NULL) {
    return DRIVER_INVALID_ARG;
  }

  int32_t temp_centi = 0;
  int32_t t_fine = 0;

  status_t status =
      bmp280_compensate_temp(raw->raw_temp, calib, &temp_centi, &t_fine);

  if (status != DRIVER_OK) {
    return status;
  }

  uint32_t press_q24_8 = 0;

  status = bmp280_compensate_press(raw->raw_press, calib, t_fine, &press_q24_8);

  if (status != DRIVER_OK) {
    return status;
  }

  sample->temp_centi = temp_centi;
  sample->press_q24_8 = press_q24_8;

  return DRIVER_OK;
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
