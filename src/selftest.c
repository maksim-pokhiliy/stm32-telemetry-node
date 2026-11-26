#include "selftest.h"
#include "conversions.h"
#include "i2c.h"
#include "status.h"
#include "uart.h"
#include <stdbool.h>
#include <stdint.h>

#define I2C_SCAN_FIRST_ADDR 0x08u
#define I2C_SCAN_LAST_ADDR 0x77u

#define I2C_TEST_CALIB_START_76 0x88u
#define I2C_TEST_CALIB_END_76 0x9Fu

#define I2C_TEST_DEV_ADDR 0x68u
#define I2C_TEST_REG 0x6Bu
#define I2C_TEST_VALUE 0x01u
#define I2C_TEST_ABSENT_ADDR 0x50u
#define I2C_TEST_EXPECTED_CHIPS 2u
#define I2C_TEST_WRITE_COUNT 100u

#define I2C_TEST_INVALID_ADDR 0xE8u

#define I2C_TEST_WHOAMI_REG_68 0x75u
#define I2C_TEST_WHOAMI_VAL_68 0x68u

#define I2C_TEST_DEV_ADDR_76 0x76u
#define I2C_TEST_WHOAMI_REG_76 0xD0u
#define I2C_TEST_WHOAMI_VAL_76 0x58u

#define I2C_TEST_PWR_REG 0x6Bu
#define I2C_TEST_PWR_VAL_INTERNAL 0x00u
#define I2C_TEST_PWR_VAL_PLL 0x01u

static void mpu_temp_selftest(void);
static void i2c_test_verdict(const char *label, bool is_passed);
static uint32_t i2c_scan(void);
static void i2c_selftest(void);

static void mpu_temp_selftest(void) {
  static const struct {
    int16_t raw;
    int32_t expected;
  } cases[] = {
      {0, 3653},     {340, 3753},   {-340, 3553},
      {-3536, 2613}, {-3504, 2622}, {-16384, -1165},
  };

  bool is_passed = true;

  uart_send_message("temp test...\r\n");

  for (uint32_t i = 0u; i < sizeof cases / sizeof cases[0]; i++) {
    int32_t centi = mpu_temp_raw_to_centi(cases[i].raw);

    uart_send_message("raw=");
    uart_send_int32(cases[i].raw);
    uart_send_message(" temp_centi=");
    uart_send_int32(centi);
    uart_send_message("\r\n");

    if (centi != cases[i].expected) {
      is_passed = false;
    }
  }

  i2c_test_verdict("temp conversion table", is_passed);
}

static void i2c_test_verdict(const char *label, bool is_passed) {
  uart_send_message(label);
  uart_send_message(is_passed ? ": PASS\r\n" : ": FAIL\r\n");
}

static uint32_t i2c_scan(void) {
  uint32_t found_count = 0u;
  uint32_t nack_count = 0u;
  uint32_t error_count = 0u;

  uart_send_message("scanning...\r\n");

  for (uint8_t address = I2C_SCAN_FIRST_ADDR; address <= I2C_SCAN_LAST_ADDR;
       address++) {
    status_t probe_status = i2c_probe(address);

    if (probe_status == DRIVER_OK) {
      uart_send_value("found 0x", address);
      uart_send_message("\r\n");

      found_count++;
    } else if (probe_status == DRIVER_NO_ACK_ADDR) {
      nack_count++;
    } else {
      uart_send_value("ERROR 0x", address);
      uart_send_value(" status=0x", (uint32_t)probe_status);
      uart_send_message("\r\n");

      error_count++;
    }
  }

  uart_send_value("scan done: found=0x", found_count);
  uart_send_value(" nack=0x", nack_count);
  uart_send_value(" error=0x", error_count);
  uart_send_message("\r\n");

  return found_count;
}

static void i2c_selftest(void) {
  uart_send_message("selftest...\r\n");

  status_t write_status =
      i2c_write_reg(I2C_TEST_DEV_ADDR, I2C_TEST_REG, I2C_TEST_VALUE);

  uart_send_value("write status=0x", (uint32_t)write_status);
  uart_send_message("\r\n");

  i2c_test_verdict("test 1 (write to 0x68)", write_status == DRIVER_OK);

  status_t invalid_write_status =
      i2c_write_reg(I2C_TEST_INVALID_ADDR, I2C_TEST_REG, I2C_TEST_VALUE);

  uart_send_value("invalid write status=0x", (uint32_t)invalid_write_status);
  uart_send_message("\r\n");

  i2c_test_verdict("test2 (write to 0xE8)",
                   invalid_write_status == DRIVER_INVALID_ARG);

  status_t absent_status =
      i2c_write_reg(I2C_TEST_ABSENT_ADDR, I2C_TEST_REG, I2C_TEST_VALUE);

  uart_send_value("absent write status=0x", (uint32_t)absent_status);
  uart_send_message("\r\n");

  uint32_t found_after_nack = i2c_scan();

  bool is_nack_test_passed = absent_status == DRIVER_NO_ACK_ADDR &&
                             found_after_nack == I2C_TEST_EXPECTED_CHIPS;

  i2c_test_verdict("test 3 (nack on 0x50, then scan)", is_nack_test_passed);

  uint32_t recover_count_before = i2c_get_recover_count();
  uint32_t ok_count = 0u;
  uint32_t timeout_count = 0u;
  uint32_t bus_error_count = 0u;
  uint32_t other_count = 0u;

  for (uint32_t i = 0u; i < I2C_TEST_WRITE_COUNT; i++) {
    status_t status =
        i2c_write_reg(I2C_TEST_DEV_ADDR, I2C_TEST_REG, I2C_TEST_VALUE);

    if (status == DRIVER_OK) {
      ok_count++;
    } else if (status == DRIVER_TIMEOUT) {
      timeout_count++;
    } else if (status == DRIVER_BUS_ERROR) {
      bus_error_count++;
    } else {
      other_count++;
    }
  }

  uint32_t recover_count = i2c_get_recover_count() - recover_count_before;

  uart_send_value("writes: ok=0x", ok_count);
  uart_send_value(" timeout=0x", timeout_count);
  uart_send_value(" bus_error=0x", bus_error_count);
  uart_send_value(" other=0x", other_count);
  uart_send_value(" recover=0x", recover_count);
  uart_send_message("\r\n");

  bool is_series_passed =
      ok_count == I2C_TEST_WRITE_COUNT && recover_count == 0u;

  i2c_test_verdict("test 4 (100 writes)", is_series_passed);

  uint32_t recover_before_invalid = i2c_get_recover_count();

  uint8_t buf[2] = {0xAA, 0xAA};

  status_t invalid_read_addr_status =
      i2c_read_reg(I2C_TEST_INVALID_ADDR, I2C_TEST_REG, buf, 1);

  uart_send_value("invalid read addr status=0x",
                  (uint32_t)invalid_read_addr_status);

  uart_send_message("\r\n");

  status_t len_0_status = i2c_read_reg(I2C_TEST_DEV_ADDR, I2C_TEST_REG, buf, 0);

  uart_send_value("len0 status=0x", (uint32_t)len_0_status);
  uart_send_message("\r\n");

  status_t null_buf_status =
      i2c_read_reg(I2C_TEST_DEV_ADDR, I2C_TEST_REG, NULL, 1);

  uart_send_value("null buf status=0x", (uint32_t)null_buf_status);
  uart_send_message("\r\n");

  uint32_t recover_invalid = i2c_get_recover_count() - recover_before_invalid;

  uart_send_value("invalid recover=0x", recover_invalid);
  uart_send_message("\r\n");

  bool buf_unchanged = (buf[0] == 0xAA) && (buf[1] == 0xAA);

  bool is_invalid_read_passed =
      invalid_read_addr_status == DRIVER_INVALID_ARG &&
      len_0_status == DRIVER_INVALID_ARG &&
      null_buf_status == DRIVER_INVALID_ARG && buf_unchanged &&
      recover_invalid == 0u;

  i2c_test_verdict("test 5 (invalid read args)", is_invalid_read_passed);

  uint8_t val = 0;

  status_t whoami_68_status =
      i2c_read_reg(I2C_TEST_DEV_ADDR, I2C_TEST_WHOAMI_REG_68, &val, 1);

  uart_send_value("read 0x68/0x75 status=0x", (uint32_t)whoami_68_status);
  uart_send_value(" val=0x", val);
  uart_send_message("\r\n");

  i2c_test_verdict("test 6 (read 0x75 from 0x68)",
                   whoami_68_status == DRIVER_OK &&
                       val == I2C_TEST_WHOAMI_VAL_68);

  val = 0;

  status_t whoami_76_status =
      i2c_read_reg(I2C_TEST_DEV_ADDR_76, I2C_TEST_WHOAMI_REG_76, &val, 1);

  uart_send_value("read 0x76/0xD0 status=0x", (uint32_t)whoami_76_status);
  uart_send_value(" val=0x", val);
  uart_send_message("\r\n");

  i2c_test_verdict("test 7 (read 0xD0 from 0x76)",
                   whoami_76_status == DRIVER_OK &&
                       val == I2C_TEST_WHOAMI_VAL_76);

  status_t write_00_status = i2c_write_reg(I2C_TEST_DEV_ADDR, I2C_TEST_PWR_REG,
                                           I2C_TEST_PWR_VAL_INTERNAL);

  uart_send_value("write 0x00 status=0x", (uint32_t)write_00_status);
  uart_send_message("\r\n");

  uint8_t read_00 = 0;

  status_t read_00_status =
      i2c_read_reg(I2C_TEST_DEV_ADDR, I2C_TEST_PWR_REG, &read_00, 1);

  uart_send_value("read after 0x00 status=0x", (uint32_t)read_00_status);
  uart_send_value(" val=0x", read_00);
  uart_send_message("\r\n");

  status_t write_01_status =
      i2c_write_reg(I2C_TEST_DEV_ADDR, I2C_TEST_PWR_REG, I2C_TEST_PWR_VAL_PLL);

  uart_send_value("write 0x01 status=0x", (uint32_t)write_01_status);
  uart_send_message("\r\n");

  uint8_t read_01 = 0;

  status_t read_01_status =
      i2c_read_reg(I2C_TEST_DEV_ADDR, I2C_TEST_PWR_REG, &read_01, 1);

  uart_send_value("read after 0x01 status=0x", (uint32_t)read_01_status);
  uart_send_value(" val=0x", read_01);
  uart_send_message("\r\n");

  bool is_rw_passed =
      write_00_status == DRIVER_OK && read_00_status == DRIVER_OK &&
      read_00 == I2C_TEST_PWR_VAL_INTERNAL && write_01_status == DRIVER_OK &&
      read_01_status == DRIVER_OK && read_01 == I2C_TEST_PWR_VAL_PLL;

  i2c_test_verdict("test 8 (write/read 0x6B)", is_rw_passed);

  uint8_t absent_buf = 0xAA;

  status_t absent_read_status =
      i2c_read_reg(I2C_TEST_ABSENT_ADDR, I2C_TEST_REG, &absent_buf, 1);

  uart_send_value("absent read status=0x", (uint32_t)absent_read_status);
  uart_send_value(" val=0x", absent_buf);
  uart_send_message("\r\n");

  uint32_t found_after_absent_read = i2c_scan();

  bool is_absent_read_passed =
      absent_read_status == DRIVER_NO_ACK_ADDR && absent_buf == 0xAA &&
      found_after_absent_read == I2C_TEST_EXPECTED_CHIPS;

  i2c_test_verdict("test 9 (nack on read 0x50, then scan)",
                   is_absent_read_passed);

  uint32_t recover_before_reads = i2c_get_recover_count();
  uint32_t read_ok = 0u;
  uint32_t read_bad_val = 0u;
  uint32_t read_fail = 0u;

  for (uint32_t i = 0u; i < I2C_TEST_WRITE_COUNT; i++) {
    uint8_t v = 0;
    status_t st =
        i2c_read_reg(I2C_TEST_DEV_ADDR, I2C_TEST_WHOAMI_REG_68, &v, 1);

    if (st == DRIVER_OK && v == I2C_TEST_WHOAMI_VAL_68) {
      read_ok++;
    } else if (st == DRIVER_OK) {
      read_bad_val++;
    } else {
      read_fail++;
    }
  }

  uint32_t recover_reads = i2c_get_recover_count() - recover_before_reads;

  uart_send_value("reads: ok=0x", read_ok);
  uart_send_value(" bad_val=0x", read_bad_val);
  uart_send_value(" fail=0x", read_fail);
  uart_send_value(" recover=0x", recover_reads);
  uart_send_message("\r\n");

  bool is_reads_passed = read_ok == I2C_TEST_WRITE_COUNT && recover_reads == 0u;

  i2c_test_verdict("test 10 (100 reads 0x75)", is_reads_passed);

  uint32_t recover_before_pairs = i2c_get_recover_count();
  uint32_t pair_ok = 0u;
  uint32_t pair_fail = 0u;

  for (uint32_t i = 0u; i < I2C_TEST_WRITE_COUNT; i++) {
    status_t wst = i2c_write_reg(I2C_TEST_DEV_ADDR, I2C_TEST_PWR_REG,
                                 I2C_TEST_PWR_VAL_PLL);

    uint8_t v = 0;
    status_t rst = i2c_read_reg(I2C_TEST_DEV_ADDR, I2C_TEST_PWR_REG, &v, 1);

    if (wst == DRIVER_OK && rst == DRIVER_OK && v == I2C_TEST_PWR_VAL_PLL) {
      pair_ok++;
    } else {
      pair_fail++;
    }
  }

  uint32_t recover_pairs = i2c_get_recover_count() - recover_before_pairs;

  uart_send_value("pairs: ok=0x", pair_ok);
  uart_send_value(" fail=0x", pair_fail);
  uart_send_value(" recover=0x", recover_pairs);
  uart_send_message("\r\n");

  bool is_pairs_passed = pair_ok == I2C_TEST_WRITE_COUNT && recover_pairs == 0u;

  i2c_test_verdict("test 11 (100 write/read pairs)", is_pairs_passed);

  uint8_t reg_vals[24] = {0};
  bool passes_match = true;
  bool all_ok = true;
  bool has_different = false;

  for (int pass = 0; pass < 2; pass++) {
    for (uint8_t address = I2C_TEST_CALIB_START_76;
         address <= I2C_TEST_CALIB_END_76; address++) {
      uint8_t addr_ind = address - I2C_TEST_CALIB_START_76;
      uint8_t addr_val_curr = 0;

      status_t result =
          i2c_read_reg(I2C_TEST_DEV_ADDR_76, address, &addr_val_curr, 1);

      if (result != DRIVER_OK) {
        all_ok = false;
      }

      if (pass == 0) {
        reg_vals[addr_ind] = addr_val_curr;
      } else {
        if (addr_val_curr != reg_vals[addr_ind]) {
          passes_match = false;
        }
      }
    }
  }

  for (uint8_t i = 1u; i < 24u; i++) {
    if (reg_vals[i] != reg_vals[0]) {
      has_different = true;

      break;
    }
  }

  uart_send_value("calib match=0x", passes_match);
  uart_send_value(" ok=0x", all_ok);
  uart_send_value(" different=0x", has_different);
  uart_send_message("\r\n");

  bool is_calib_passed = passes_match && all_ok && has_different;

  i2c_test_verdict("test 12 (calib 0x88-0x9F twice)", is_calib_passed);

  uint8_t etalon[24] = {0};
  bool etalon_ok = true;

  for (uint8_t i = 0u; i < 24u; i++) {
    status_t st = i2c_read_reg(I2C_TEST_DEV_ADDR_76,
                               I2C_TEST_CALIB_START_76 + i, &etalon[i], 1);
    if (st != DRIVER_OK) {
      etalon_ok = false;
    }
  }

  uint32_t recover_before_win = i2c_get_recover_count();
  bool win_ok = true;
  bool guard_ok = true;
  bool match_ok = true;

  for (uint8_t offset = 0u; offset < 23u; offset++) {
    uint8_t buf[3] = {0xAA, 0xAA, 0xAA};

    status_t st = i2c_read_reg(I2C_TEST_DEV_ADDR_76,
                               I2C_TEST_CALIB_START_76 + offset, buf, 2);

    if (st != DRIVER_OK) {
      win_ok = false;
    }

    if (buf[0] != etalon[offset] || buf[1] != etalon[offset + 1]) {
      match_ok = false;
    }

    if (buf[2] != 0xAA) {
      guard_ok = false;
    }
  }

  uint32_t recover_win = i2c_get_recover_count() - recover_before_win;

  uart_send_value("win2: etalon_ok=0x", etalon_ok);
  uart_send_value(" win_ok=0x", win_ok);
  uart_send_value(" match=0x", match_ok);
  uart_send_value(" guard=0x", guard_ok);
  uart_send_value(" recover=0x", recover_win);
  uart_send_message("\r\n");

  bool is_win2_passed =
      etalon_ok && win_ok && match_ok && guard_ok && recover_win == 0u;

  i2c_test_verdict("test 13 (2-byte windows on calib)", is_win2_passed);

  uint8_t etalon3[24] = {0};
  bool etalon3_ok = true;

  for (uint8_t i = 0u; i < 24u; i++) {
    status_t st = i2c_read_reg(I2C_TEST_DEV_ADDR_76,
                               I2C_TEST_CALIB_START_76 + i, &etalon3[i], 1);
    if (st != DRIVER_OK) {
      etalon3_ok = false;
    }
  }

  uint32_t recover_before_win3 = i2c_get_recover_count();
  bool win3_ok = true;
  bool guard3_ok = true;
  bool match3_ok = true;

  for (uint8_t offset = 0u; offset < 22u; offset++) {
    uint8_t buf[4] = {0xAA, 0xAA, 0xAA, 0xAA};

    status_t st = i2c_read_reg(I2C_TEST_DEV_ADDR_76,
                               I2C_TEST_CALIB_START_76 + offset, buf, 3);

    if (st != DRIVER_OK) {
      win3_ok = false;
    }

    if (buf[0] != etalon3[offset] || buf[1] != etalon3[offset + 1] ||
        buf[2] != etalon3[offset + 2]) {
      match3_ok = false;
    }

    if (buf[3] != 0xAA) {
      guard3_ok = false;
    }
  }

  uint32_t recover_win3 = i2c_get_recover_count() - recover_before_win3;

  uart_send_value("win3: etalon_ok=0x", etalon3_ok);
  uart_send_value(" win_ok=0x", win3_ok);
  uart_send_value(" match=0x", match3_ok);
  uart_send_value(" guard=0x", guard3_ok);
  uart_send_value(" recover=0x", recover_win3);
  uart_send_message("\r\n");

  bool is_win3_passed =
      etalon3_ok && win3_ok && match3_ok && guard3_ok && recover_win3 == 0u;

  i2c_test_verdict("test 14 (3-byte windows on calib)", is_win3_passed);

  uint8_t etalon4[24] = {0};
  bool etalon4_ok = true;

  for (uint8_t i = 0u; i < 24u; i++) {
    status_t st = i2c_read_reg(I2C_TEST_DEV_ADDR_76,
                               I2C_TEST_CALIB_START_76 + i, &etalon4[i], 1);
    if (st != DRIVER_OK) {
      etalon4_ok = false;
    }
  }

  uint32_t recover_before_win4 = i2c_get_recover_count();
  bool win4_ok = true;
  bool guard4_ok = true;
  bool match4_ok = true;

  for (uint8_t offset = 0u; offset < 21u; offset++) {
    uint8_t buf[5] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA};

    status_t st = i2c_read_reg(I2C_TEST_DEV_ADDR_76,
                               I2C_TEST_CALIB_START_76 + offset, buf, 4);

    if (st != DRIVER_OK) {
      win4_ok = false;
    }

    if (buf[0] != etalon4[offset] || buf[1] != etalon4[offset + 1] ||
        buf[2] != etalon4[offset + 2] || buf[3] != etalon4[offset + 3]) {
      match4_ok = false;
    }

    if (buf[4] != 0xAA) {
      guard4_ok = false;
    }
  }

  uint32_t recover_win4 = i2c_get_recover_count() - recover_before_win4;

  uart_send_value("win4: etalon_ok=0x", etalon4_ok);
  uart_send_value(" win_ok=0x", win4_ok);
  uart_send_value(" match=0x", match4_ok);
  uart_send_value(" guard=0x", guard4_ok);
  uart_send_value(" recover=0x", recover_win4);
  uart_send_message("\r\n");

  bool is_win4_passed =
      etalon4_ok && win4_ok && match4_ok && guard4_ok && recover_win4 == 0u;

  i2c_test_verdict("test 15 (4-byte windows on calib)", is_win4_passed);

  uint8_t etalon24[24] = {0};
  bool etalon24_ok = true;

  for (uint8_t i = 0u; i < 24u; i++) {
    status_t st = i2c_read_reg(I2C_TEST_DEV_ADDR_76,
                               I2C_TEST_CALIB_START_76 + i, &etalon24[i], 1);
    if (st != DRIVER_OK) {
      etalon24_ok = false;
    }
  }

  uint32_t recover_before_full = i2c_get_recover_count();

  uint8_t buf24[25];

  for (uint8_t i = 0u; i < 25u; i++) {
    buf24[i] = 0xAA;
  }

  status_t full_st =
      i2c_read_reg(I2C_TEST_DEV_ADDR_76, I2C_TEST_CALIB_START_76, buf24, 24);

  bool full_match = true;

  for (uint8_t i = 0u; i < 24u; i++) {
    if (buf24[i] != etalon24[i]) {
      full_match = false;
    }
  }

  bool full_guard = (buf24[24] == 0xAA);
  uint32_t recover_full = i2c_get_recover_count() - recover_before_full;

  uart_send_value("full24: etalon_ok=0x", etalon24_ok);
  uart_send_value(" st=0x", (uint32_t)full_st);
  uart_send_value(" match=0x", full_match);
  uart_send_value(" guard=0x", full_guard);
  uart_send_value(" recover=0x", recover_full);
  uart_send_message("\r\n");

  bool is_full24_passed = etalon24_ok && full_st == DRIVER_OK && full_match &&
                          full_guard && recover_full == 0u;

  i2c_test_verdict("test 16 (24-byte read calib)", is_full24_passed);

  uint8_t etalon_16a[3] = {0};
  bool etalon_16a_ok = true;

  for (uint8_t i = 0u; i < 3u; i++) {
    status_t st = i2c_read_reg(I2C_TEST_DEV_ADDR_76,
                               I2C_TEST_CALIB_START_76 + i, &etalon_16a[i], 1);
    if (st != DRIVER_OK) {
      etalon_16a_ok = false;
    }
  }

  uint8_t buf_16a_2[3] = {0xAA, 0xAA, 0xAA};

  status_t st_16a_2 =
      i2c_read_reg(I2C_TEST_DEV_ADDR_76, I2C_TEST_CALIB_START_76, buf_16a_2, 2);

  uint8_t buf_16a_1 = 0xAA;

  status_t st_16a_1 = i2c_read_reg(I2C_TEST_DEV_ADDR_76,
                                   I2C_TEST_CALIB_START_76 + 2, &buf_16a_1, 1);

  bool match_16a = (buf_16a_2[0] == etalon_16a[0]) &&
                   (buf_16a_2[1] == etalon_16a[1]) &&
                   (buf_16a_1 == etalon_16a[2]) && (buf_16a_2[2] == 0xAA);

  uart_send_value("16a: st2=0x", (uint32_t)st_16a_2);
  uart_send_value(" st1=0x", (uint32_t)st_16a_1);
  uart_send_value(" match=0x", match_16a);
  uart_send_message("\r\n");

  i2c_test_verdict("test 17a (2 then 1)",
                   etalon_16a_ok && st_16a_2 == DRIVER_OK &&
                       st_16a_1 == DRIVER_OK && match_16a);

  uint8_t etalon_16b[5] = {0};
  bool etalon_16b_ok = true;

  for (uint8_t i = 0u; i < 5u; i++) {
    status_t st = i2c_read_reg(I2C_TEST_DEV_ADDR_76,
                               I2C_TEST_CALIB_START_76 + i, &etalon_16b[i], 1);
    if (st != DRIVER_OK) {
      etalon_16b_ok = false;
    }
  }

  uint8_t buf_16b_2[3] = {0xAA, 0xAA, 0xAA};

  status_t st_16b_2 =
      i2c_read_reg(I2C_TEST_DEV_ADDR_76, I2C_TEST_CALIB_START_76, buf_16b_2, 2);

  uint8_t buf_16b_3[4] = {0xAA, 0xAA, 0xAA, 0xAA};

  status_t st_16b_3 = i2c_read_reg(I2C_TEST_DEV_ADDR_76,
                                   I2C_TEST_CALIB_START_76 + 2, buf_16b_3, 3);

  bool match_16b =
      (buf_16b_2[0] == etalon_16b[0]) && (buf_16b_2[1] == etalon_16b[1]) &&
      (buf_16b_3[0] == etalon_16b[2]) && (buf_16b_3[1] == etalon_16b[3]) &&
      (buf_16b_3[2] == etalon_16b[4]) && (buf_16b_2[2] == 0xAA) &&
      (buf_16b_3[3] == 0xAA);

  uart_send_value("16b: st2=0x", (uint32_t)st_16b_2);
  uart_send_value(" st3=0x", (uint32_t)st_16b_3);
  uart_send_value(" match=0x", match_16b);
  uart_send_message("\r\n");

  i2c_test_verdict("test 17b (2 then 3)",
                   etalon_16b_ok && st_16b_2 == DRIVER_OK &&
                       st_16b_3 == DRIVER_OK && match_16b);

  uint8_t buf_16c[3] = {0xAA, 0xAA, 0xAA};

  status_t st_16c_read =
      i2c_read_reg(I2C_TEST_DEV_ADDR_76, I2C_TEST_CALIB_START_76, buf_16c, 2);

  status_t st_16c_write =
      i2c_write_reg(I2C_TEST_DEV_ADDR, I2C_TEST_PWR_REG, I2C_TEST_PWR_VAL_PLL);

  uint8_t readback_16c = 0xAA;

  status_t st_16c_readback =
      i2c_read_reg(I2C_TEST_DEV_ADDR, I2C_TEST_PWR_REG, &readback_16c, 1);

  uart_send_value("16c: read76=0x", (uint32_t)st_16c_read);
  uart_send_value(" write68=0x", (uint32_t)st_16c_write);
  uart_send_value(" readback=0x", (uint32_t)st_16c_readback);
  uart_send_value(" val=0x", readback_16c);
  uart_send_message("\r\n");

  i2c_test_verdict("test 17c (2 from 0x76 then write/read 0x68)",
                   st_16c_read == DRIVER_OK && st_16c_write == DRIVER_OK &&
                       st_16c_readback == DRIVER_OK &&
                       readback_16c == I2C_TEST_PWR_VAL_PLL &&
                       buf_16c[2] == 0xAA);

  uint8_t etalon_17a = 0;

  status_t etalon_17a_st = i2c_read_reg(
      I2C_TEST_DEV_ADDR_76, I2C_TEST_CALIB_START_76, &etalon_17a, 1);

  uint8_t buf_17a[3] = {0xAA, 0xAA, 0xAA};

  status_t st_17a_nack =
      i2c_read_reg(I2C_TEST_ABSENT_ADDR, I2C_TEST_CALIB_START_76, buf_17a, 2);

  uint8_t buf_17a_ok = 0xAA;

  status_t st_17a_ok = i2c_read_reg(I2C_TEST_DEV_ADDR_76,
                                    I2C_TEST_CALIB_START_76, &buf_17a_ok, 1);

  bool nack_ok_17a = (st_17a_nack == DRIVER_NO_ACK_ADDR) &&
                     (buf_17a[0] == 0xAA) && (buf_17a[1] == 0xAA) &&
                     (buf_17a[2] == 0xAA);

  uart_send_value("17a: nack=0x", (uint32_t)st_17a_nack);
  uart_send_value(" ok=0x", (uint32_t)st_17a_ok);
  uart_send_value(" val=0x", buf_17a_ok);
  uart_send_message("\r\n");

  i2c_test_verdict("test 18a (2 from 0x50 then 1 from 0x76)",
                   etalon_17a_st == DRIVER_OK && nack_ok_17a &&
                       st_17a_ok == DRIVER_OK && buf_17a_ok == etalon_17a);

  uint8_t etalon_17b[3] = {0};
  bool etalon_17b_ok = true;

  for (uint8_t i = 0u; i < 3u; i++) {
    status_t st = i2c_read_reg(I2C_TEST_DEV_ADDR_76,
                               I2C_TEST_CALIB_START_76 + i, &etalon_17b[i], 1);
    if (st != DRIVER_OK) {
      etalon_17b_ok = false;
    }
  }

  uint8_t buf_17b_nack[4] = {0xAA, 0xAA, 0xAA, 0xAA};

  status_t st_17b_nack = i2c_read_reg(I2C_TEST_ABSENT_ADDR,
                                      I2C_TEST_CALIB_START_76, buf_17b_nack, 3);

  uint8_t buf_17b_1 = 0xAA;

  status_t st_17b_1 = i2c_read_reg(I2C_TEST_DEV_ADDR_76,
                                   I2C_TEST_CALIB_START_76, &buf_17b_1, 1);

  uint8_t buf_17b_2[3] = {0xAA, 0xAA, 0xAA};

  status_t st_17b_2 = i2c_read_reg(I2C_TEST_DEV_ADDR_76,
                                   I2C_TEST_CALIB_START_76 + 1, buf_17b_2, 2);

  uint32_t found_17b = i2c_scan();

  bool nack_ok_17b = (st_17b_nack == DRIVER_NO_ACK_ADDR) &&
                     (buf_17b_nack[0] == 0xAA) && (buf_17b_nack[1] == 0xAA) &&
                     (buf_17b_nack[2] == 0xAA) && (buf_17b_nack[3] == 0xAA);

  bool after_ok_17b = (st_17b_1 == DRIVER_OK && buf_17b_1 == etalon_17b[0]) &&
                      (st_17b_2 == DRIVER_OK && buf_17b_2[0] == etalon_17b[1] &&
                       buf_17b_2[1] == etalon_17b[2] && buf_17b_2[2] == 0xAA);

  uart_send_value("17b: nack=0x", (uint32_t)st_17b_nack);
  uart_send_value(" st1=0x", (uint32_t)st_17b_1);
  uart_send_value(" st2=0x", (uint32_t)st_17b_2);
  uart_send_value(" found=0x", found_17b);
  uart_send_message("\r\n");

  i2c_test_verdict("test 18b (3 from 0x50 then 1+2 from 0x76 + scan)",
                   etalon_17b_ok && nack_ok_17b && after_ok_17b &&
                       found_17b == I2C_TEST_EXPECTED_CHIPS);

  uint8_t etalon18[24] = {0};
  bool etalon18_ok = true;

  for (uint8_t i = 0u; i < 24u; i++) {
    status_t st = i2c_read_reg(I2C_TEST_DEV_ADDR_76,
                               I2C_TEST_CALIB_START_76 + i, &etalon18[i], 1);
    if (st != DRIVER_OK) {
      etalon18_ok = false;
    }
  }

  uint32_t recover_before_18 = i2c_get_recover_count();
  uint32_t ok_18 = 0u;
  uint32_t mismatch_18 = 0u;
  uint32_t fail_18 = 0u;

  for (uint32_t n = 0u; n < I2C_TEST_WRITE_COUNT; n++) {
    uint8_t buf[24] = {0};

    status_t st =
        i2c_read_reg(I2C_TEST_DEV_ADDR_76, I2C_TEST_CALIB_START_76, buf, 24);

    if (st != DRIVER_OK) {
      fail_18++;
      continue;
    }

    bool match = true;

    for (uint8_t i = 0u; i < 24u; i++) {
      if (buf[i] != etalon18[i]) {
        match = false;

        break;
      }
    }

    if (match) {
      ok_18++;
    } else {
      mismatch_18++;
    }
  }

  uint32_t recover_18 = i2c_get_recover_count() - recover_before_18;

  uart_send_value("full100: ok=0x", ok_18);
  uart_send_value(" mismatch=0x", mismatch_18);
  uart_send_value(" fail=0x", fail_18);
  uart_send_value(" recover=0x", recover_18);
  uart_send_message("\r\n");

  bool is_full100_passed =
      etalon18_ok && ok_18 == I2C_TEST_WRITE_COUNT && recover_18 == 0u;

  i2c_test_verdict("test 19 (100x 24-byte calib)", is_full100_passed);
}

void selftest_run(void) {
  mpu_temp_selftest();
  (void)i2c_scan();
  i2c_selftest();
}
