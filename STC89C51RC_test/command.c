/***********************************************************
## Command interrupt handler

buf[] contains null-terminated message
buf_proc should be cleared at the end.

*/
#ifdef UART

__code const char *err_strs[] = {
  "OK",                // 0
  "unknown command: ", // 1
  "command missing",   // 2
  "argument missing",  // 3
  "i2c: no device",    // 4 nack when sending device address
  "i2c: no register",  // 5 nack when sending register
  "i2c: write error",  // 6 nack when sending bytes
};

void
command_int(void) __interrupt (IE0_VECTOR) {
  char *tok;
  unsigned char errno = 0;

  // First word
  tok = strtok(buf, " ");
  if (tok == NULL){ errno = 2; goto msg_end; }

  /**********************************************/

  // `*idn?` or `*IDN?` -- print device ID
  if (strcmp(tok,"*idn?")==0 || strcmp(tok,"*IDN?")==0){
    put_str(DEVICE_IDN);
    put_char('\n');
    goto msg_end;
  }

  /**********************************************/
  // I2C commands
  #ifdef I2C

  /* `i2c <scl_pin> <sda_pin> <address> <count> [<byte to send> ...]` -- 
     I2C communication. I2C is using `scl_pin` and `sda_pin` of `P2` register
     as SCL and SDA lines. Device address is `address`. If there are some 
     `<bytes to send>`, then a write sequence is performed first. Normally,
     if you want to read a value from some register, a register number should
     be sent here. Then, if `count` is not zero, then a read sequence is
     performed for `count` bytes with ACK response after all bytes except the
     last one, and NACK response after the last byte.
  */

  if (strcmp(tok,"i2c")==0){
    unsigned char addr, reg, scl_pin, sda_pin;
    unsigned int count, i;

    tok = strtok(NULL, " ");
    if (tok == NULL){ errno=3; goto msg_end; }
    scl_pin = atoi(tok);
    tok = strtok(NULL, " ");
    if (tok == NULL){ errno=3; goto msg_end; }
    sda_pin = atoi(tok);

    tok = strtok(NULL, " ");
    if (tok == NULL){ errno=3; goto msg_end; }
    addr = atoi(tok);
    tok = strtok(NULL, " ");
    if (tok == NULL){ errno=3; goto msg_end; }
    count = atoi(tok);

    // Do we want to write some bytes before reading?
    tok = strtok(NULL, " ");
    if (tok != NULL){
      i2c_start(scl_pin, sda_pin);
      if (!i2c_send(addr << 1)){
        i2c_stop(); errno=4; goto msg_end; }

      while (tok != NULL) {
        reg = atoi(tok);
        if (!i2c_send(reg)){
          i2c_stop(); errno=5; goto msg_end; }
        tok = strtok(NULL, " ");
      }
      i2c_stop();
    }

    // get data
    if (count>0){
      i2c_start(scl_pin, sda_pin);
      if (!i2c_send((addr << 1) + 1u)){
        i2c_stop(); errno=4; goto msg_end; }
      for (i = 0; i<count; i++){
        if (i>0) put_char(' ');
        put_hex_byte(i2c_get(i+1 < count)); // nack at last byte
      }
      i2c_stop();
      put_char('\n');
    }

    goto msg_end;
  }

  # endif

  /**********************************************/
  // Display commands
  #ifdef I2C
  #ifdef DISPLAY

  // display_init -- init display
  if (strcmp(tok,"display_init")==0){
    display_init();
    goto msg_end;
  }

  // display (1|0) -- switch display ON/OFF:
  if (strcmp(tok,"display")==0){
    tok = strtok(NULL, " ");
    if (tok == NULL){ errno=3; goto msg_end; }
    display_enable(atoi(tok));
    goto msg_end;
  }

  // display_clear -- clear display
  if (strcmp(tok,"display_clear")==0){
    display_clear();
    goto msg_end;
  }

  // print ASCII font starting from index N: display_test <N>
  if (strcmp(tok,"display_test")==0){
    tok = strtok(NULL, " ");
    if (tok == NULL){ errno=3; goto msg_end; }
    display_x=0;
    display_y=0;
    for (int i = 0; i<64; i++){
      int v = i + atoi(tok);
      display_putc(v);
    }
    goto msg_end;
  }

  // Change cursor position: display_goto <x> <y>
  if (strcmp(tok,"display_goto")==0){
    char x,y;
    tok = strtok(NULL, " ");
    if (tok == NULL){ errno=3; goto msg_end; }
    x = atoi(tok);
    tok = strtok(NULL, " ");
    if (tok == NULL){ errno=3; goto msg_end; }
    y = atoi(tok);
    display_x=x;
    display_y=y;
    goto msg_end;
  }

  // Print string: display_puts <string>
  // Escape sequencies supported: \n for CR+NL,
  // for other characters \<x> prints x.
  // NOTE: spaces in the beginning of the argumet are eaten by strtok(), use \t
  if (strcmp(tok,"display_puts")==0){
    tok = strtok(NULL, ""); // do not split on spaces!
    if (tok == NULL){ return; }
    for (int i=0; i<strlen(tok); i++){
      if (tok[i] == '\\'){
        i++;
        if (tok[i] == 'n'){
          display_x=0;
          display_y = (display_y+1)%(DISPLAY_W/8);
        }
        else if (tok[i] == 't')
           display_putc(' ');
        else
           display_putc(tok[i]); // works for end 0 too
      }
      else
        display_putc(tok[i]);
    }
    goto msg_end;
  }
  #endif
  #endif

  /**********************************************/
  // BMP280 commands
  #ifdef I2C
  #ifdef BMP280

  // bmp280_init
  if (strcmp(tok,"bmp280_init")==0){
    i2c_start(BMP280_SCL, BMP280_SDA);
    if (!i2c_send(BMP280_ADDR << 1)){
      i2c_stop(); errno=4; goto msg_end; }
    if ( !i2c_send(0xF4) || !i2c_send(0xFF) // ctrl_meas
   /* || !i2c_send(0xF5) || !i2c_send(0xFF)*/ // config
       ){ i2c_stop(); errno=6; goto msg_end; }
    i2c_stop();
    goto msg_end;
  }

  // bmp280_cal -- print calibration data (T1..T3, P1..P9)
  if (strcmp(tok,"bmp280_cal")==0){
    i2c_start(BMP280_SCL, BMP280_SDA);
    if (!i2c_send(BMP280_ADDR << 1)){
      i2c_stop(); errno=4; goto msg_end; }
    if (!i2c_send(0x88)){
      i2c_stop(); errno=5; goto msg_end; }
    i2c_stop();

    i2c_start(BMP280_SCL, BMP280_SDA);
    if (!i2c_send((BMP280_ADDR << 1) + 1u)){
      i2c_stop(); errno=4; goto msg_end; }
    for (char i = 0; i<12; i++){
      put_long((i2c_get(1) << 8) + i2c_get(i!=11));
      put_char(i!=11? ' ': '\n');
    }
    i2c_stop();
    goto msg_end;
  }

  // bmp280_meas -- measure raw data (T,P)
  if (strcmp(tok,"bmp280_meas")==0){
    long int res = 0;
    i2c_start(BMP280_SCL, BMP280_SDA);
    if (!i2c_send(BMP280_ADDR << 1)){
      i2c_stop(); errno=4; goto msg_end; }
    if (!i2c_send(0xF7)){
      i2c_stop(); errno=5; goto msg_end; }
    i2c_stop();

    i2c_start(BMP280_SCL, BMP280_SDA);
    if (!i2c_send((BMP280_ADDR << 1) + 1u)){
      i2c_stop(); errno=4; goto msg_end; }

    for (int i=0; i<6; i++){
      res <<= 8;
      res += i2c_get(i!=5);
      if (i==2 || i==5) {
        res &= 0x00FFFFF0;
        put_long(res);
        put_char(i!=5? ' ': '\n');
        res = 0;
      }
    }
    i2c_stop();
    goto msg_end;
  }

  #endif
  #endif

  /**********************************************/
  errno = 1; // unknown command

  msg_end:
  if (errno==0){
    put_str("#OK\n");
  }
  else {
    put_str("#Error: ");
    put_str(err_strs[errno]);
    if (errno == 1) put_str(buf); // print unknown command
    put_char('\n');
  }
  buf_proc = 0;
  return;
}

#endif