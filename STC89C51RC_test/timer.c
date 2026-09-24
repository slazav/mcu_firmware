/***********************************************************
## Timer0 interrupt handler

Interrups is triggered every millisecond. An additional counter is used
to count TIMER_MS intervals.

*/

# define TIME_MAX 60000l // timer cycle in ms, 1min
__data long int time = 0; // timer counter (ms)

void timer0_int(void) __interrupt (TF0_VECTOR) {
   TH0 = 0xFC; TL0 = 0x18; // reload timer (1 ms = 0x10000 - 1000)

   time++;
   if (time > TIME_MAX){ time = 0; }

   /**** place for payload ****/

   if (time%10000 == 0) {
     put_str("timer: ");
     put_long(time/1000);
     put_str(" s\n");
   }

   /****/


//if (time%5000 == 0){
//  i2c_comm(0x3C); // SSD1306
//i2c_comm_w(0x76, 0xF4, 0xFF); // BMP280
//i2c_comm_r(0x76, 0xF7, 3); // BMP280
//i2c_comm_r(0x76, 0xF8, 2); // BMP280
//i2c_comm_r(0x76, 0xF9, 1); // BMP280
//i2c_comm_r(0x76, 0xFA, 3); // BMP280
//i2c_comm_r(0x76, 0xFB, 2); // BMP280
//i2c_comm_r(0x76, 0xFC, 1); // BMP280
//}
}
