## STC89C51RC_test

Firmware architecture:

- Device is in idle mode with three interrupts enabled.

- High-priority UART interrupt handler which reads incoming characters to
  a buffer and when a line is completed emits interrupt for
  command processing.

- Command-processing handler reads command messages,
  performs some actions and writes replies.

- Optional timer interrupt is emitted avery 1ms, some periodic actions
  can be put here.

Code is arranged as a collection of blocks which can be included separately
by defining flags in main.c:

- TIMER -- timer subsystem: 1ms timer interrupt
- UART -- uart subsystem
- I2C - i2c subsystem
- DISPLAY - display subsystem (needs I2C)
- FONT_ASCII7 - ASCII7 font for display (0x20..0x7F, 96 chars, 768 bytes)
- FONT_KOI8_R - KOI8_R font for display (0xC0..0xFF, 64 chars, 512 bytes)
- BMP280 -- basic commands for bmp280 pressure/temperature sensor

Tools needed:

- sdcc compiler
- stcgal program for uploading firmware

#### Terminal commands

Each command is a single line. Length should be smaller then BUF_MAX which is currently
defined as 64. Error message is printed if command is too long. After each response either
`#OR` or `#Error: <text>` line is printed. All characters received while a command is
being processed are ignored.

*  `*idn?` or `*IDN?` -- prints device ID which is currently defined as STC89_SLAZAV1

*  `i2c <scl_pin> <sda_pin> <address> <count> [<byte to send> ...]` --
I2C communication. I2C is using `scl_pin` and `sda_pin` of `P2` register
as SCL and SDA lines. Device address is `address`. If there are some
`<bytes to send>`, then a write sequence is performed first. Normally,
if you want to read a value from some register, a register number should
be sent here. Then, if `count` is not zero, then a read sequence is
performed for `count` bytes with ACK response after all bytes except the
last one, and NACK response after the last byte.

* `display_init` -- Initialize display.
* `display (0|1)` -- Display ON/OFF
* `display_clear` -- clear display, set cursor to the top-left corner
* `display_goto <x> <y>` -- move cursor
* `display_test <N>` -- print font starting from index N
* `display_puts` -- print a string. \n can be used for CR+NL, for other characters \<x> prints x.
NOTE: spaces in the beginning of the argumet are eaten, use `\ ` to keep them.

* `bmp280_init` -- init BMP device (fixed configuration with maximum sampling)
* `bmp280_cal` -- get calibration data (T1..T3, P1..P9)
* `bmp280_meas` -- get raw data for temperature and pressure
