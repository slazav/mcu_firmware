/***********************************************************
* Display

***********************************************************/

#define DISPLAY_H    32
#define DISPLAY_W    128
#define DISPLAY_SCL  1
#define DISPLAY_SDA  0
#define DISPLAY_ADDR 0x3Cu

#ifdef FONT_ASCII7
  #include "font_ascii7.inc"
#endif
#ifdef FONT_KOI8_R
  #include "font_koi8_r.inc"
#endif

char display_x, display_y; // cursor position

void
display_init(void) {

  __xdata char cmds[] = {
    0xAE,       // display off
    0x20, 0x00, // memory address mode (0 - horizontal, 1 - vertical, 2 - page)
    0x40,       // display start line = 0
    0xA1,       // segment re-map, column 127 is mapped to SEG0
    0xA8, DISPLAY_H-1, // set multiplex ratio
    0xC8,       // set COM (common) output scan direction. Scan from bottom up, COM[N-1] to COM0
    0xD3, 0x00, // set display offset
    0xDA, 0x02, // set common pins hardware configuration.  0x02 for 128x32, 0x12 for 128x64
    0xD5, 0x80, // display clock divide ratio (0x80 - div ratio of 1, standard freq)
    0xD9, 0xF1, // pre-charge periods
    0xDB, 0x30, // VCOMH deselect level (0.83xVcc)
    0x81, 0x7F, // contrast
    0xA4,       // display follows RAM content
    0xA6,       // non-inverted display
    0x8D, 0x14, // set charge pump (Vcc internally generated on our board)
//    0x2E,       // deactivate scrolling if set
    0xAF        // display on
  };

  i2c_start(DISPLAY_SCL,DISPLAY_SDA);
  i2c_send(DISPLAY_ADDR << 1);
  for (int i=0; i<sizeof(cmds); i++){
    i2c_send(0x80);
    i2c_send(cmds[i]);
  }
  i2c_stop();
  display_x=0;
  display_y=0;
}

void
display_enable(__bit v) {
  i2c_start(DISPLAY_SCL,DISPLAY_SDA);
  i2c_send(DISPLAY_ADDR << 1);
  i2c_send(0x80);
  i2c_send(0xAE + v);
  i2c_stop();
}

void
display_clear(void) {
  __xdata char cmds[] = {
    0x21, 0x00, 0x7F, // column range 0..127
    0x22, 0x00, 0x07  // page range 0..7
  };

  i2c_start(DISPLAY_SCL,DISPLAY_SDA);
  i2c_send(DISPLAY_ADDR << 1);
  for (int i=0; i<sizeof(cmds); i++){
    i2c_send(0x80);
    i2c_send(cmds[i]);
  }
  i2c_send(0x40);
  for (int i = 0; i< 128*8; i++) i2c_send(0x0);
  i2c_send(0x80);
  i2c_stop();
  display_x=0;
  display_y=0;
}

// print 8x8 character at the current cursor position
void
display_putc(char v) {
   if (display_x>DISPLAY_W/8 ||
       display_y>DISPLAY_H/8) return; // out of screen

   // default character
   __code const char def_char[] = {0xff,0x81,0x81,0x81,0x81,0x81,0x81,0xff};
   __code const char *out = &def_char;

   #ifdef FONT_ASCII7
   if (v>=0x20 && v<=0x7F) out = display_font_ascii7[v-0x20];
   #endif
   #ifdef FONT_KOI8_R
   if (v>=0xC0) out = display_font_koi8_r[v-0xC0];
   #endif

  // set update region
  char cmds[] = {
    0x21, display_x<<3, 0x7F, //column range 0..127
    0x22, display_y, 7, //page range 0..7
  };
  i2c_start(DISPLAY_SCL,DISPLAY_SDA);
  i2c_send(DISPLAY_ADDR << 1);
  for (int i=0; i<sizeof(cmds); i++){
    i2c_send(0x80);
    i2c_send(cmds[i]);
  }

  // send data
  i2c_send(0x40);
  for (int i = 0; i< 8; i++)
    i2c_send(out[i]);

  i2c_stop();

  // move cursor
  if (display_x == DISPLAY_W/8 - 1)
    display_y = (display_y+1)%(DISPLAY_W/8);
  display_x = (display_x+1)%(DISPLAY_W/8);
}

