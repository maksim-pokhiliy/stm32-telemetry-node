#ifndef MPU6050_H
#define MPU6050_H

#include "status.h"
#include <stdint.h>

typedef struct {
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;
  int16_t temp;
  int16_t gyro_x;
  int16_t gyro_y;
  int16_t gyro_z;
} mpu6050_sample_t;

status_t mpu6050_init(void);
status_t mpu6050_read_sample(mpu6050_sample_t *sample);

#endif
