#include "defines.h"
#include "uart.c"
#include "vid.c"

int color;
char *tab = "0123456789ABCDEF";

/*typedef struct uart{
  char *base;
  int  n;
}UART;*/

//extern UART uart[4];

extern char _binary_Clock_bmp_start;
extern char _binary_Clock_bmp_end;

int color;
UART *up;

int main()
{
  char c, *p;
  int mode, i = 1;
  uart_init();
  up = &uart[0];

  mode = 0;
  fbuf_init(mode);

  p = &_binary_Clock_bmp_start;
  show_bmp(p, 0, 0, i); 
  i++;

  while(1){
    fuprintf("enter a key from this UART : ");
    ugetc(up);
    p = &_binary_Clock_bmp_start;
    show_bmp(p, 0, 0, i);
    i++;
    if(i > 3)
      i = 0;
  }
   //while(1);   // loop here  
}
