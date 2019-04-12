/*******************************************************
*                  @t.c file                          *
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

int prints(char *s)
{
    while(*s)
        putc(*s++);
}

int gets(char *s)
{ 
    while((*s = getc()) != '\r')
        putc(*s++);
    *s = 0;
}

u16 NSEC = 2;
char buf1[BLK], buf2[BLK];

u16 getblk(u16 blk, char *buf)
{
    readfd( (2*blk)/CYL, ( (2*blk)%CYL)/TRK, ((2*blk)%CYL)%TRK, buf);
    // readfd( blk/18, ((blk)%18)/9, ( ((blk)%18)%9)<<1, buf);
}

u16 search(INODE *ip, char *name)
{
    /*search for name in the data block of INODE; 
    return its inumber if found
    else error();*/
    int i;
    char c;
    DIR *dp;

    for(i=0; i<12; i++) // enter 12 direct block to check name
    {
        if ((u16)ip->i_block[i])    // only enter if the block contain data
        {
            getblk((u16)ip->i_block[i], buf2);
            dp = (DIR *)buf2;
            while((char *)dp < &buf2[BLK])
            {
                c = dp->name[dp->name_len];
                dp->name[dp->name_len] = 0;
                prints(dp->name); prints("  ");

                if(strcmp(dp->name, name) == 0)
                {;
                    dp->name[dp->name_len] = c;
                    return (u16)dp->inode;
                }

                dp->name[dp->name_len] = c;
                dp = (char *)dp + dp->rec_len;
            }
        }
    }
    error();
    return 0;
}

int main()
{ 
    u16 i, iblk, ino;
    u32 *up;
    char c, temp[64], *name[2];
    GD *gp;
    INODE *ip;

    //1. Write YOUR C code to get the INODE of /boot/mtx
    //INODE *ip --> INODE
    name[0] = "boot"; 
    name[1] = "mtx";

    prints("booting mtx... \n\r");
    getblk(2, buf1);

    gp = (GD *)buf1;
    iblk = (u16)gp->bg_inode_table;

    getblk(iblk, buf1);
    ip = (INODE *)buf1 + 1;

    // search for system name
    for(i = 0; i<2; i++)
    {
        ino = search(ip, name[i]) - 1;
        if(ino < 0)
            error();
        getblk(iblk + (ino/8), buf1);
        ip = (INODE *)buf1 + (ino % 8);
    }

    //if INODE has indirect blocks: get i_block[12] int buf2[  ]
    if ((u16)ip->i_block[12])
        getblk((u16)ip->i_block[12], buf2);

    //2. setes(0x1000);  // MTX loading segment = 0x1000
    setes(0x1000);

    //3. load 12 DIRECT blocks of INODE into memory
    for(i=0; i<12; i++){
        getblk((u16)ip->i_block[i], 0);
        putc('*');
        inces();
    }

    //4. load INDIRECT blocks, if any, into memory
    if ((u16)ip->i_block[12]){
        up = (u32 *)buf2;      
        while(*up){
            getblk((u16)*up, 0); putc('.');
            inces();
            up++;
        }
    }

    prints("go?"); getc();
}  
