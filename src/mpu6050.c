#include "mpu6050.h"
#include "i2c.h"
#include "status.h"
#include "systick.h"
#include <stddef.h>
#include <stdint.h>

#define MPU6050_ADDR 0x68u

#define MPU6050_REG_SMPLRT_DIV 0x19u
#define MPU6050_REG_CONFIG 0x1Au
#define MPU6050_REG_GYRO_CONFIG 0x1Bu
#define MPU6050_REG_ACCEL_CONFIG 0x1Cu
#define MPU6050_REG_ACCEL_XOUT_H 0x3Bu
#define MPU6050_REG_PWR_MGMT_1 0x6Bu
#define MPU6050_REG_PWR_MGMT_2 0x6Cu
#define MPU6050_REG_WHO_AM_I 0x75u

#define MPU6050_WHO_AM_I_VALUE 0x68u
#define MPU6050_PWR_MGMT_1_WAKE_PLL 0x01u
#define MPU6050_PWR_MGMT_2_ALL_ON 0x00u
#define MPU6050_CONFIG_DLPF_OFF 0x00u
#define MPU6050_SMPLRT_DIV_NONE 0x00u
#define MPU6050_GYRO_CONFIG_250DPS 0x00u
#define MPU6050_ACCEL_CONFIG_2G 0x00u

#define MPU6050_WAKE_DELAY_MS 100u
#define MPU6050_SAMPLE_LEN 14u

_Static_assert((int16_t)(uint16_t)0x8000u == -32768,
               "uint16_t -> int16_t must wrap modulo 2^16");

_Static_assert((int16_t)(uint16_t)0xF220u == -3552,
               "uint16_t -> int16_t must wrap modulo 2^16");

static int16_t mpu6050_make_int16(uint8_t high, uint8_t low);

static int16_t mpu6050_make_int16(uint8_t high, uint8_t low) {
  uint16_t raw = (uint16_t)(((uint16_t)high << 8) | low);

  return (int16_t)raw;
}

status_t mpu6050_init(void) {
  uint8_t who_am_i = 0u;

  status_t status =
      i2c_read_reg(MPU6050_ADDR, MPU6050_REG_WHO_AM_I, &who_am_i, 1);

  if (status != DRIVER_OK) {
    return status;
  }

  if (who_am_i != MPU6050_WHO_AM_I_VALUE) {
    return DRIVER_WRONG_ID;
  }

  status = i2c_write_reg(MPU6050_ADDR, MPU6050_REG_PWR_MGMT_1,
                         MPU6050_PWR_MGMT_1_WAKE_PLL);

  if (status != DRIVER_OK) {
    return status;
  }

  systick_delay_ms(MPU6050_WAKE_DELAY_MS);

  status = i2c_write_reg(MPU6050_ADDR, MPU6050_REG_PWR_MGMT_2,
                         MPU6050_PWR_MGMT_2_ALL_ON);
  if (status != DRIVER_OK) {
    return status;
  }

  status =
      i2c_write_reg(MPU6050_ADDR, MPU6050_REG_CONFIG, MPU6050_CONFIG_DLPF_OFF);
  if (status != DRIVER_OK) {
    return status;
  }

  status = i2c_write_reg(MPU6050_ADDR, MPU6050_REG_SMPLRT_DIV,
                         MPU6050_SMPLRT_DIV_NONE);
  if (status != DRIVER_OK) {
    return status;
  }

  status = i2c_write_reg(MPU6050_ADDR, MPU6050_REG_GYRO_CONFIG,
                         MPU6050_GYRO_CONFIG_250DPS);
  if (status != DRIVER_OK) {
    return status;
  }

  status = i2c_write_reg(MPU6050_ADDR, MPU6050_REG_ACCEL_CONFIG,
                         MPU6050_ACCEL_CONFIG_2G);
  if (status != DRIVER_OK) {
    return status;
  }

  return DRIVER_OK;
}

status_t mpu6050_read_sample(mpu6050_sample_t *sample) {
  if (sample == NULL) {
    return DRIVER_INVALID_ARG;
  }

  uint8_t raw[MPU6050_SAMPLE_LEN];

  status_t status = i2c_read_reg(MPU6050_ADDR, MPU6050_REG_ACCEL_XOUT_H, raw,
                                 MPU6050_SAMPLE_LEN);

  if (status != DRIVER_OK) {
    return status;
  }

  sample->accel_x = mpu6050_make_int16(raw[0], raw[1]);
  sample->accel_y = mpu6050_make_int16(raw[2], raw[3]);
  sample->accel_z = mpu6050_make_int16(raw[4], raw[5]);
  sample->temp = mpu6050_make_int16(raw[6], raw[7]);
  sample->gyro_x = mpu6050_make_int16(raw[8], raw[9]);
  sample->gyro_y = mpu6050_make_int16(raw[10], raw[11]);
  sample->gyro_z = mpu6050_make_int16(raw[12], raw[13]);

  return DRIVER_OK;
}
