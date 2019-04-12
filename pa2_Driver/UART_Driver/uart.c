#define DR   0x00
#define FR   0x18

#define RXFE 0x10
#define TXFF 0x20

typedef struct uart{
    char *base;
    int n;
}UART;

UART uart[4];
char *ctable = "0123456789ABCDEF";
int base10 = 10, baseHEX = 16;

int uart_init()
{
    int i; UART *up;

    for (i=0; i<4; i++){
        up = &uart[i];
        up->base = (char *)(0x1000A000 + i*0x1000);
        up->n = i;
    }
    uart[3].base = (char *)(0x10009000); // uart3 at 0x10009000
}

int ugetc(UART *up)
{
    while (*(up->base + FR) & RXFE);
    return *(up->base + DR);
}

int uputc(UART *up, char c)
{
    while(*(up->base + FR) & TXFF);
    *(up->base + DR) = c;
}

int ugets(UART *up, char *s)
{
    while ((*s = (char)ugetc(up)) != '\r'){
        uputc(up, *s);
        s++;
    }
    *s = 0;
}

int uprints(UART *up, char *s)
{
    while(*s)
        uputc(up, *s++);
}

int rpu(UART *up, unsigned int x)
{
    char c;
    if(x)
    {
        c = ctable[x % base10];
        rpu(up, x / base10);
        uputc(up, c);
    }
}

int uprintu(UART *up, unsigned int x)
{
    (x==0)? uputc(up, '0'): rpu(up, x);
}

int uprintd(UART *up, int x)
{
    if(x < 0)
    {
        uputc(up, '-');
        x *= -1;
    }
    uprintu(up, x);
}

int rpuHex(UART *up, unsigned int x)
{
    char c;
    if(x)
    {
        c = ctable[x % baseHEX];
        rpuHex(up, x / baseHEX);
        uputc(up, c);
    }
}

int uprintx(UART *up, unsigned int x)
{
    uputc(up, '0');
    uputc(up, 'x');
    (x==0)? uputc(up, '0'): rpuHex(up, x); 
}


int fuprintf(UART *up, char *fmt, ...)
{
    char *cp = fmt;
    int *ip = (int *)&fmt + 1;  // address of input variables

    while(*cp)
    {
        if(*cp != '%')
            uputc(up, *cp);
        else
        {
            switch(*++cp)
            {
                case 'c':
                    uputc(up, *ip++);
                    break;
                case 'd':
                    uprintd(up, *ip++);
                    break;
                case 's':
                    uprints(up, *ip++);
                    break;
                case 'u':
                    uprintu(up, *ip++);
                    break;
                case 'x':
                    uprintx(up, *ip++);
                    break;
                case '%':
                    uputc(up, '%');
                    break;
                default:
                    break;
            }
        }
        cp++;   // go next char
    }

}
