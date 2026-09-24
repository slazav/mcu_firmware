/*
A simple firmware for my STC89C51RC controller. Not finished yet.
- Controller has 12 MHz clock and T12 mode (timers increment every 12 clocks, 1us)
- Controller is staying in idle mode with three interrupts enabled: Timer0 and UART, Ext0.
- Timer0 produces an interrupt every 1 millisecond. Interrupt handler
  has an additional counter to measure bigger time periods.
- UART is configured for 4800 baud 8E1 mode (and uses Timer1 for baud rate setting)
- Device is receiving text messages separated by \n or \n and store them in a buffer.
  Ext0 interrupt handler processes received messages.
- i2c protocol is implemented

compiling:
$ sdcc -mmcs51 $<

uploading:
$ stcgal -b 4800 serial.ihx

Note: It's convenient to use same port settings in firmware and uploader
(at least for tests). I'm using 4800 8E1

STTY settings:

speed 4800 baud; line = 0;
min = 1; time = 10;
-brkint -icrnl -imaxbel
-opost -onlcr
-isig -icanon -iexten -echo -echoe -echok -echoctl -echoke

*/


#define DEVICE_IDN "STC89_SLAZAV1" // reply for *IDN? command

//#define TIMER   // timer subsystem: 1ms timer interrupt
#define UART      // uart subsystem
#define I2C       // i2c subsystem
#define DISPLAY   // display subsystem (i2c needed)
#define FONT_ASCII7 // ASCII7 font (0x20..0x7F, 96 chars, 768 bytes)
//#define FONT_KOI8_R // KOI8_R font (0xC0..0xFF, 64 chars, 512 bytes)
#define BMP280    // BMP280 subsystem (i2c needed)

#define BMP280_SCL  6
#define BMP280_SDA  7
#define BMP280_ADDR 0x76u

#include <mcs51/stc89.h> // registers
#include <string.h>
#include <stdlib.h>

#ifdef UART
  #include "uart.c"
#endif

#ifdef TIMER
  #include "timer.c"
#endif

#ifdef I2C
  #include "i2c.c"

  #ifdef DISPLAY
    #include "display.c"
  #endif

#endif


#include "command.c"

/***********************************************************/
void
main(void) {

    #ifdef TIMER
    // Timer0 setting
    ET0 = 1; // Enable Timer0 interrupt
    TMOD |= 0x01; // 16bit
    TH0 = 0xFC; TL0 = 0x18; // set timer rate to 1ms (0x10000 - 1000 = 0xFC18)
    TR0  = 1; // Timer0 ON
    #endif

    #ifdef UART
    // Timer1 and UART settings
    TMOD |= 0x20; // Timer1 - 8bit, auto-reload
    PCON |= 0x80; // double the baud rate (SMOD=1)
    // baud rate =  2^SMOD/32 * (SYSCLOCK/12) / (0x100 - TH1) )
    // TH1  baud   std   error
    // 0xFF 62500  57600  +8.5%
    // 0xFD 20833  19200  +8.5%
    // 0xFA 10417   9600  +8.5%
    // 0xF9 8929    9600  -7.0%
    // 0xF3 4808    4800  +1.7% -- only this works for me
    TH1 = 0xF3;  // set baud rate
    TL1 = TH1;   // auto-reload value
    TR1  = 1;    // Timer1 ON

    EX0 = 1; // Enable external interrupt 0
    IT0 = 1; // Edge trigger type
    ET1 = 0; // Disable Timer1 interrupt
    ES  = 1; // Enable UART interrupt
    IP |= 0x10; // high priority for UART interrupts

    SCON = 0x50; // 8N1 UART with variable baud rate
    #endif

    EA  = 1; // Enable interrupts

    // go to idle mode
    PCON |= 0x01;
}

