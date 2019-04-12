typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

/*
UART0 base address: 0x101f1000;
UART1 base address: 0x101f2000;
UART2 base address: 0x101f3000;
UART3 base address: 0x10009000;

// flag register at 0x18
//  7    6    5    4    3    2   1   0
// TXFE RXFF TXFF RXFE BUSY
// TX FULL : 0x20
// TX empty: 0x80
// RX FULL : 0x40
// RX empty: 0x10
// BUSY=1 :  0x08
*/

int N;
int v[] = {1,2,3,4,5,6,7,8,9,10};
int sum;

char *tab = "0123456789ABCDEF";

#include "string.c"
#include "uart.c"

UART *up;

int main()
{
    int i;
    int size = sizeof(int);
    char string[32]; 
    char line[128]; 

    N = 10;

    uart_init();

    for(i=0; i<4; i++)
    {
        up = &uart[i];
        fuprintf(up, "%s", "enter a line from this UART : ");
        ugets(up, string);
        if(strcmp(string, "quit") == 0)
            break;
        fuprintf(up, "    ECHO %s\n", string);

        if(i == 3)
            i = -1;
    }

    // change 3 code //
    /*for(i=0; i<4; i++)
    {
        up = &uart[i];
        uprints(up, "Enter a line from this UART : ");
        ugets(up, string);
        uprints(up, "     ECHO : "); uprints(up, string); uprints(up, "\n\r");
    }

    // original code //
    /*up = &uart[0];
    uprints(up, "Enter lines from UART terminal, enter quit to exit\n\r");
  
    while(1){
        ugets(up, string);
        uprints(up, "    ");
        if (strcmp(string, "quit")==0)
            break;
        uprints(up, string);  uprints(up, "\n\r");
    }*/


    //uprints(up, "Compute sum of array\n\r");
    fuprintf(up, "\n\nCompute sum of array\n");
    sum = 0;
    for (i=0; i<N; i++)
        sum += v[i];
    //uputc(up, (sum/10)+'0'); uputc(up, (sum%10)+'0');
    fuprintf(up, "sum = %d\n", sum);
    //uprints(up, "\n\rEND OF RUN\n\r");
    fuprintf(up, "\nEND OF RUN\n");
}
