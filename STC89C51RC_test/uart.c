/***********************************************************/
// IO functions

int
put_char(int c) {
    SBUF = c; // start transmission
    while(TI==0); // wait for it
    TI = 0;
    return (c);
}

void
put_str(const char *str) {
  for (int i=0; i<strlen(str); i++){
    SBUF = str[i]; // start transmission
    while(TI==0); // wait for it
    TI = 0;
  }
}

void
put_long(long int n) {
  long int d;
  if (n<0) { put_char('-'); n = -n;}
  // max 9 digits
  for (d = 10; d<1000000000l; d*=10) {
    if (n/d==0) break;
  }
  for (d/=10; d>0; d/=10){
    put_char((n/d)%10 + '0');
  }
}

__code const char hex_str[] = "0123456789ABCDEF";

void
put_hex_byte(unsigned char c) {
  put_str("0x");
  put_char(hex_str[(c>>4) & 0x0F]);
  put_char(hex_str[c & 0x0F]);
}

void
put_hex_word(unsigned int c) {
  put_str("0x");
  put_char(hex_str[(c>>12) & 0x0F]);
  put_char(hex_str[(c>>8) & 0x0F]);
  put_char(hex_str[(c>>4) & 0x0F]);
  put_char(hex_str[c & 0x0F]);
}

/***********************************************************
## UART interrupt handler

Receiving messages separated be \n or \r, appending characters to buf[] array.
Triggering Int0 when message is received. Handler should clear buf_proc bit.

If a character is received when old message is not processed yet, it is ignored.

*/

#define BUF_MAX 64   // Input buffer length
__data char buf[BUF_MAX];
__data char buf_n = 0; // current buffer position
__bit buf_proc = 0; // set when message is ready, cleared then message is processed
__bit skip = 0;     // if set, skip characters until end of line

void
uart_int(void) __interrupt (SI0_VECTOR) {
   if (RI==0) return; // not interested in transmission interrupts

   __data char c = SBUF;
   RI = 0;  // clear interrupt flag

   // buffer is not processed yet, skip characters
   if (buf_proc) {skip = 1; return; }

   // end of line
   if (c == '\n' || c == '\r'){
     skip = 0;
     if (buf_n == 0) return; // skip \n\r in the beginning of a message
     buf[buf_n] = '\0';
     buf_n = 0;
     buf_proc = 1;
     IE0=1; // message is ready
     return;
   }

   // overflow error (leave space for \0)
   if (buf_n >= BUF_MAX-1) {
     put_str("#Error: buffer overflow\n");
     skip = 1;
     buf_n = 0;
     return;
   }

   if (skip) return;

   buf[buf_n] = c;
   buf_n++;
}

