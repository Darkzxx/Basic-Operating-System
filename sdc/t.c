/********************************************************************
Copyright 2010-2017 K.C. Wang, <kwang@eecs.wsu.edu>
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************/

#include "type.h"
#include "string.c"
#define VA(x) (0x80000000 + (u32)x)

char *tab = "0123456789ABCDEF";
int BASE;
int color;


#include "kbd.c"

#include "vid.c"
#include "exceptions.c"
#include "queue.c"
#include "kernel.c"
#include "sdc.c"


void copy_vectors(void) {
    extern u32 vectors_start;
    extern u32 vectors_end;
    u32 *vectors_src = &vectors_start;
    u32 *vectors_dst = (u32 *)0;
    while(vectors_src < &vectors_end)
       *vectors_dst++ = *vectors_src++;
}

int kprintf(char *fmt, ...);

void IRQ_handler()
{
    int vicstatus, sicstatus;
    int ustatus, kstatus;

    // read VIC status register to find out which interrupt
    vicstatus = VIC_STATUS;
    sicstatus = SIC_STATUS;  

    if (vicstatus & (1<<31)){
      if (sicstatus & (1<<3)){
          kbd_handler();
       }
       // SDC interrupt at 22 on SIC
       if (sicstatus & (1<<22)){
          sdc_handler();
       }
    }
}


char buf[1024], buf1[1024], buf2[1204];
int bmap, imap, iblk;
int blk;

GD    *gp;
INODE *ip;
DIR   *dp;

char *cp;
char c;

int main()
{ 
   int i; 
   
   color = RED;
   row = col = 0; 
   BASE = 10;
      
   fbuf_init();
   kprintf("                     Welcome to WANIX in Arm\n");
   kprintf("LCD display initialized : fbuf = %x\n", fb);
   color = CYAN;
   kbd_init();
   
   VIC_INTENABLE |= (1<<31);    // SIC to VIC's IRQ31

   /* enable KBD and SDC IRQ */
   SIC_ENSET    |= (1<<3);   // KBD int=3 on SIC
   SIC_ENSET    |= (1<<22);  // SDC int=22 on SIC
   
   sdc_init();
   kernel_init();
        
   unlock();
   color = WHITE;
   printf("P0 running: test SDC driver and EXT2 FS on sdimge \n");
    
   getblock(2, buf);
   gp = (GD *)buf;
   bmap = gp->bg_block_bitmap;
   imap = gp->bg_inode_bitmap;
   iblk = gp->bg_inode_table;
   printf("bmp=%d  imbp=%d  iblk=%d\n", bmap, imap, iblk);

   getblock(iblk, buf);
   ip = (INODE *)buf + 1;
   blk = ip->i_block[0];
   printf("i_block[0] = %d\n", ip->i_block[0]);
   printf("ino r_len n_len  name\n");
   
   getblock(blk, buf2);

   dp = (DIR *)buf2;
   cp = buf2;
   while (cp < buf2 + 1024){
      c = dp->name[dp->name_len];  // save last byte

      dp->name[dp->name_len] = 0;   
    printf("%d   %d    %d   %s\n",
	   dp->inode, dp->rec_len, dp->name_len, dp->name); 

      dp->name[dp->name_len] = c; // restore that last byte
      cp += dp->rec_len;
      dp = (DIR *)cp;
   }

   while(1);
}
