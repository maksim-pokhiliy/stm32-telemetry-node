#include "conversions.h"
#include <stdint.h>

#define MPU_TEMP_CENTI_PER_DEGREE 100
#define MPU_TEMP_OFFSET_CENTI 3653
#define MPU_TEMP_LSB_PER_DEGREE 340

int32_t mpu_temp_raw_to_centi(int16_t raw) {
  int32_t numerator = (int32_t)raw * MPU_TEMP_CENTI_PER_DEGREE +
                      (int32_t)MPU_TEMP_OFFSET_CENTI * MPU_TEMP_LSB_PER_DEGREE;

  return numerator / MPU_TEMP_LSB_PER_DEGREE;
}
