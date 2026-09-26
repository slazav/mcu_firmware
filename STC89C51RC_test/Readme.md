## STC89C51RC_test

Firmware architecture:

- Device is in idle mode with three interrupts enabled.

- High-priority UART interrupt handler which reads incoming characters to
  a buffer and when a line is completed emits interrupt for
  command processing.

- Command-processing handler reads command messages,
  performs some actions and writes replies.

- Optional timer interrupt is emitted every 1ms, some periodic actions
  can be put here.

Code is arranged as a collection of blocks which can be included separately
by defining flags in main.c:

- TIMER -- timer subsystem: 1ms timer interrupt
- UART -- uart subsystem
- I2C - i2c subsystem
- DISPLAY - display subsystem for SSD1306-based 128x32 OLED screen (needs I2C)
- FONT_ASCII7 - ASCII7 font for display (0x20..0x7F, 96 chars, 768 bytes)
- FONT_KOI8_R - KOI8_R font for display (0xC0..0xFF, 64 chars, 512 bytes)
- BMP280 -- basic commands for bmp280 pressure/temperature sensor

Tools needed:

- sdcc compiler
- stcgal program for uploading firmware

#### Terminal commands

Each command is a single line. Length should be smaller then BUF_MAX which is currently
defined as 64. Error message is printed if command is too long. After each response either
`#OR` or `#Error: <text>` line is printed. Commands received while a previous command is
being processed are ignored.

*  `*idn?` or `*IDN?` -- prints device ID which is currently defined as STC89_SLAZAV1.
I'm using this command to identify and configure the device in udev rule.

*  `i2c <scl_pin> <sda_pin> <address> <count> [<byte to send> ...]` --
Universal I2C communication (optional read after optional write). I2C is
using `scl_pin` and `sda_pin` of `P2` register as SCL and SDA lines.
Device address is `address`. If there are some `<bytes to send>`, then a
write sequence is performed first. Normally, if you want to read a value
from some register, a register number should be sent here. Then, if
`count` is not zero, then a read sequence is performed for `count` bytes
with ACK response after all bytes except the last one, and NACK response
after the last byte.

* `display_init` -- Initialize display.
* `display (0|1)` -- Display ON/OFF
* `display_clear` -- clear display, set cursor to the top-left corner
* `display_goto <x> <y>` -- move cursor
* `display_test <N>` -- print font starting from index N
* `display_puts <text>` -- print a string. \n can be used for CR+NL, for other characters \<x> prints x.
The argument can contain spaces, but all spaces in the beginning are eaten, use `\ ` to keep them

* `bmp280_init` -- init BMP device (fixed configuration with maximum sampling)
* `bmp280_cal` -- get calibration data (T1..T3, P1..P9)
* `bmp280_meas` -- get raw data for temperature and pressure


#### Integration with udev and device2

My board uses CH340 serial converter which does not have unique ID. To
distinguish it from my other devices with the same chip I'm using
`device_ping` tool from my device2 package (https://github.com/slazav/device2).

udev rule:
```
#1a86:7523 QinHeng Electronics CH340 serial converter (STC89 mcu)
ACTION=="add", SUBSYSTEM=="tty",\
   ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523",\
   PROGRAM+="/etc/udev/ping_stc89 %k",\
   GROUP="users", MODE="0660", SYMLINK+="stc89"
```

Script `/etc/udev/ping_stc89`:

```
#!/bin/sh

device_ping serial -dev "/dev/$1" -speed 4800 -parity 8N1 -sfc 0\
  -vmin 1 -timeout 10 -add_str "\n" -trim_str "\n" -spp 1\
  -- ""

device_ping serial -dev "/dev/$1" -speed 4800 -parity 8N1 -sfc 0\
  -vmin 1 -timeout 10 -add_str "\n" -trim_str "\n" -spp 1\
  -- "*idn?" | grep -q "^STC89"

```

Here the serial port is configured, *idn? command is sent to the device,
the answer affects the return code. Unfortunately, I can not avoid
power-up garbage in UART, so I'm sending one more command to neutralize
it.

Configuration in `/etc/device2/devices.cfg`:
```
stc  serial -dev "/dev/stc89" -speed 4800 -parity 8N1 -sfc 0\
  -ndelay 0 -raw 1 -vmin 1 -add_str "\n" -trim_str "\n" -spp 1
```

