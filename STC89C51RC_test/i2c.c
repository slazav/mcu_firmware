/***********************************************************/
// I2C test

__data int i2c_delay = 2; // 120 = 1 ms

// Masks
__data char scl0, scl1, sda0, sda1;

#define I2C_DEL \
__asm \
  nop \
__endasm;

/*
// 1ms = 120 loops
static void delay(void){
  char j;
  for(j = 0; j < i2c_delay; j++);
}
*/

// Define i2c pins (in P2 register), start I2C communication
void
i2c_start(char scl_pin, char sda_pin){
  scl1 = 1 << scl_pin;
  sda1 = 1 << sda_pin;
  scl0 = ~scl1;
  sda0 = ~sda1;

  // stop condition if needed
  P2 |= scl1; I2C_DEL
  P2 |= sda1; I2C_DEL

  // start condition
  P2 &= sda0; I2C_DEL
  P2 &= scl0; I2C_DEL
}

// stop I2C communication
void
i2c_stop(void){
  P2 &= sda0; I2C_DEL
  P2 |= scl1; I2C_DEL
  P2 |= sda1; I2C_DEL
}

// write a byte, read and return ACK
__bit
i2c_send(unsigned char b){
  __bit ack;
  for (signed char i = 7; i >= 0; i--) {
    if ((b >> i) & 1) P2 |= sda1;
    else P2 &= sda0;
                I2C_DEL
    P2 |= scl1; I2C_DEL
                I2C_DEL
    P2 &= scl0; I2C_DEL
  }
  // read ACK
  P2 |= sda1; I2C_DEL
  P2 |= scl1; I2C_DEL
  ack = !(P2 & sda1);
              I2C_DEL
  P2 &= scl0; I2C_DEL
  return ack;
}

// get a byte, send ack/nack
unsigned char
i2c_get(__bit ack){
  char i;
  unsigned char ret = 0;
  P2 |= sda1; I2C_DEL
  for (i = 0; i < 8; i++) {
    P2 |= scl1; I2C_DEL
    ret = ret << 1;
    if (P2 & sda1) ret |= 1;
                I2C_DEL
    P2 &= scl0; I2C_DEL
  }
  // send ACK/NACK
  if (ack) P2 &= sda0;
  else P2 != sda1;
              I2C_DEL
  P2 |= scl1; I2C_DEL
              I2C_DEL
  P2 &= scl0; I2C_DEL
  P2 |= sda1; I2C_DEL
  return ret;
}

