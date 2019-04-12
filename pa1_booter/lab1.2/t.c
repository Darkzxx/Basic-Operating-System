/*******************************************************
*                      t.c file                        *
*******************************************************/
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u32;

#define TRK 18
#define CYL 36
#define BLK 1024

#include "ext2.h"

typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;
GD    *gp;
INODE *ip;
DIR   *dp;
char  *cp;

char buf1[BLK], buf2[BLK];
int color = 0x0A;
u8 ino;

int prints(char *s)
{
    // write YOUR code
    while(*s)
        putc(*s++);
}

int gets(char *s)
{ 
    // write YOUR code
    while((*s = getc()) != '\r')
        putc(*s++);
    *s = 0;
}

int wait()
{
    char c = 0;
    while(c == 0)
        c = getc();
}

int getblk(u16 blk, char *buf)
{
    //readfd( (2*blk)/CYL, ( (2*blk)%CYL)/TRK, ((2*blk)%CYL)%TRK, buf);
    readfd( blk/18, ((blk)%18)/9, ( ((blk)%18)%9)<<1, buf);
}



int main()
{ 
    u16 i, iblk;
    char c, temp[64];

    prints("read block# 2 (GD)\n\r");
    getblk(2, buf1);

    // 1. WRITE YOUR CODE to get iblk = bg_inode_table block number
    gp = (GD *)buf1;
    iblk = (u16)gp->bg_inode_table;

    wait();

    prints("inode_block="); putc(iblk+'0'); prints("\n\r"); 

    // 2. WRITE YOUR CODE to get root inode
    prints("read inodes begin block to get root inode\n\r");

    getblk(iblk, buf1);
    ip = (INODE *)buf1 + 1;
    wait();

    for(i=0; i<12; i++)
    {
        if(ip->i_block[i])
        {
            prints("i_block["); putc(i+'0'); prints("] = ");
            putc((u16)ip->i_block[i]+'0'); prints("\n\r");
        }
    }

    // 3. WRITE YOUR CODE to step through the data block of root inode
    prints("read data block of root DIR\n\r");
    getblk((u16)ip->i_block[0], buf1);

    // 4. print file names in the root directory /
    dp = (DIR *)buf1;
    //prints("enter while loop \n\r");
    while((char *)dp < &buf1[BLK])
    {
        prints("  ");
        c = dp->name[dp->name_len];
        dp->name[dp->name_len] = '\0';
        
        prints(dp->name);

        dp->name[dp->name_len] = c;
        dp = (char *)dp + dp->rec_len;
                
        wait();
    }
    
    prints("\n\rall done");
}  

