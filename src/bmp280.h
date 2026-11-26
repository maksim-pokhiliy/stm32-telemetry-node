#ifndef BMP280_H
#define BMP280_H

#include "status.h"
#include <stdint.h>

typedef struct {
  uint16_t dig_T1;
  int16_t dig_T2;
  int16_t dig_T3;
  uint16_t dig_P1;
  int16_t dig_P2;
  int16_t dig_P3;
  int16_t dig_P4;
  int16_t dig_P5;
  int16_t dig_P6;
  int16_t dig_P7;
  int16_t dig_P8;
  int16_t dig_P9;
} bmp280_calib_t;

typedef struct {
  uint32_t raw_press;
  uint32_t raw_temp;
} bmp280_raw_sample_t;

status_t bmp280_init(void);
const bmp280_calib_t *bmp280_get_calibration(void);
status_t bmp280_read_raw(bmp280_raw_sample_t *sample);

status_t bmp280_compensate_temp(uint32_t raw_temp, const bmp280_calib_t *calib,
                                int32_t *temp_centi, int32_t *t_fine);

#endif
