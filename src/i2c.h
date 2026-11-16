#ifndef I2C_H
#define I2C_H

#include "status.h"
#include <stdbool.h>
#include <stdint.h>

status_t i2c_init(void);
status_t i2c_probe(uint8_t address);

#endif
