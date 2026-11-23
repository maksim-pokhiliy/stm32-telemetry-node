#ifndef I2C_H
#define I2C_H

#include "status.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

status_t i2c_init(void);
status_t i2c_probe(uint8_t address);
status_t i2c_write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t value);

status_t i2c_read_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *buf,
                      size_t len);

uint32_t i2c_get_recover_count(void);

#endif
