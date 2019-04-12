#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

#include "type.h"

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern OFT  oft[NOFT];
extern char gpath[256];
extern char *name[64]; // assume at most 64 components in pathnames
extern int  n;
extern int  fd, dev;
extern int  nblocks, ninodes, bmap, imap, inode_start;
extern char pathname[256], parameter[256];
extern const char *filemode[];

// Check whether the file is ALREADY opened with INCOMPATIBLE mode:
// return the mode it's already open with or -1 for havn't been open
int checkOFT(MINODE *mip)
{
    int i;
    for(i=0; i < NOFT; i++)
    {
        if(oft[i].mptr == 0)
            continue;
        
        if((oft[i].mptr->dev == mip->dev) && (oft[i].mptr->ino == mip->ino))
        {
            if((oft[i].refCount > 0) && (oft[i].mode > -1))
                return oft[i].mode;
        }
    }
    return -1;
}

// check permission access
int hasPermission(MINODE *mip, int access)
{
    if((mip->INODE.i_mode & access) != 0)
        return 1;
    return 0;
}

int my_truncate(MINODE *mip)
{
    int i;

    for(i=0; i < 15; i++)
    {
        if(!mip->INODE.i_block[i])
            continue;
        bdalloc(mip->dev, mip->INODE.i_block[i]);
    }

    // update INODE's time field
    mip->INODE.i_atime = time(0L);
    mip->INODE.i_mtime = time(0L);

    // set INODE's size to 0 and mark MINODE dirty
    mip->INODE.i_size = 0;
    mip->dirty = 1;
}


// function to open file
// mode = 0|1|2|3 for R|W|RW|APPEND
int open_file()
{
    int mode = -1, ino, f_mode, i ,fd;
    int u_perm = 0, g_perm = 0;
    MINODE *mip;
    OFT *oftp;

    // check pathname is given
    if(pathname[0] == 0 || parameter[0] == 0)
    {
        printf("FORMAT ERROR: open (string)pathname (string|int)mode\n");
        printf("mode: R|W|RW|AP  or  0|1|2|3\n");
        return -1;
    }

    // update dev on given pathname
    if(pathname[0] == '/')
        dev = root->dev;
    else
        dev = running->cwd->dev;
    
    // check if pathname exists
    if((ino = getino(pathname)) == 0)
        return -1;
    
    mip = iget(dev, ino);

    // get mode from parameter
    for(i=0; i < 4; i++)
    {
        if(!strcmp(parameter, filemode[i]))
        {
            mode = i;
            break;
        }
    }
    if(mode == -1)
        mode = atoi(parameter);

    // incorrect mode
    if(mode < 0 || mode > 3)
    {
        printf("ERROR: incorrect mode input\n");
        printf("mode: R|W|RW|AP  or  0|1|2|3\n");
        return -1;
    }

    // check it's a regular file
    if(!S_ISREG(mip->INODE.i_mode))
    {
        printf("ERROR: %s is not a regular file\n", basename(pathname));
        iput(mip);
        return -1;
    }

    // check user's permission
    if(running->uid == mip->INODE.i_uid)
        u_perm = 1;
    if(running->uid == mip->INODE.i_gid)
        g_perm = 1;

    // check if the file is ALREADY opened with INCOMPATIBLE mode:
    // REJECT: if it's already open for W|RW|AP
    f_mode = checkOFT(mip);
    printf("f_mode = %d\n", f_mode);

    if(f_mode > 0)          // already open for W|RW|AP
    {
        printf("ERROR: %s already opened for %s\n", basename(pathname), filemode[f_mode]);
        iput(mip);
        return -1;
    }
    else if(f_mode == 0)    // already opened for R
    {
        // only multiple R mode is ok
        if(mode == 0)
        {
            if(!(u_perm && hasPermission(mip, S_IRUSR)) && !(g_perm && hasPermission(mip, S_IRGRP))
                && !hasPermission(mip, S_IROTH))
            {
                printf("ERROR: R permission denied\n");
                iput(mip);
                return -1;
            }
        }
        else
        {
            printf("ERROR: %s already opened for %s\n", basename(pathname), filemode[f_mode]);
            iput(mip);
            return -1;
        }
    }
    else                // not yet open
    {
        // check access permission
        if(mode == 0)
        {
            if(!(u_perm && hasPermission(mip, S_IRUSR)) && !(g_perm && hasPermission(mip, S_IRGRP))
                && !hasPermission(mip, S_IROTH))
            {
                printf("ERROR: R permission denied\n");
                iput(mip);
                return -1;
            }
        }
        else if(mode == 1 || mode == 3)
        {
            if(!(u_perm && hasPermission(mip, S_IWUSR)) && !(g_perm && hasPermission(mip, S_IWGRP))
                && !hasPermission(mip, S_IWOTH))
            {
                printf("ERROR: W|AP permission denied\n");
                iput(mip);
                return -1;
            }
        }
        else
        {
            if(!(u_perm && hasPermission(mip, 384)) && !(g_perm && hasPermission(mip, 48))
                && !hasPermission(mip, 6))
            {
                printf("ERROR: RW permission denied\n");
                iput(mip);
                return -1;
            }
        }
    }
    printf("permission passed\n");

    // allocate oft and get oftp
    for(i=0; i < NOFT; i++)
    {
        if(oft[i].mptr == 0)
        {
            oftp = &oft[i];
            break;
        }
    }

    // return if oft is filled up
    if(i == NOFT)
    {
        printf("ERROR: open file table is currently filled\n");
        iput(mip);
        return -1;
    }
    printf("oft[i] found\n");

    // fills in values
    oftp->mode = mode;
    oftp->mptr = mip;
    oftp->refCount = 1;
    printf("mode = %d\n", mode);

    // set the OFT's offset depending to mode
    switch(mode)
    {
        case 1: my_truncate(mip);   // W: truncate file to size 0
            oftp->offset = 0;
            break;
        case 2: oftp->offset = 0;   // RW: not truncating the file
            break;
        case 3: oftp->offset = mip->INODE.i_size; // AP mode
            break;
        default: oftp->offset = 0;  // R: offset = 0
            break;
    }
    
    // let running->fd[i] point at the OFT entry
    for(i = 0; i < NFD; i++)
    {
        if(!running->fd[i])
        {
            running->fd[i] = oftp;
            break;
        }
    }

    if(i >= NFD)
    {
        printf("ERROR: No available fd[i] in running process\n");
        iput(mip);
        return -1;
    }

    // update INODE's time field
    mip->INODE.i_atime = time(0L);
    // for R: only touch atime else also mtime
    if(mode)
        mip->INODE.i_mtime = time(0L);

    // mark dirty
    mip->dirty = 1;
    iput(mip);

    // return file descripter i
    return i;
}

// close file
int close_file()
{
    int i, fd;
    OFT *oftp;
    MINODE *mip;

    // check pathname is given
    if(pathname[0] == 0)
    {
        printf("FORMAT ERROR: close (int)file_descripter\n");
        printf("see available file descripter by using command: pfd\n");
        return -1;
    }

    fd = atoi(pathname);
    printf("close fd = %d\n", fd);
    // verify fd is within range
    if(fd < 0 || fd >= NFD)
    {
        printf("ERROR: fd is out of range\n");
        return -1;
    }

    // verify running->fd[fd] is pointing at a OFT entry
    if(running->fd[fd] == 0)
    {
        printf("ERROR: There is no %d fd\n", fd);
        return -1;
    }
    // verify that it also exist in oft
    for(i=0; i < NOFT; i++)
    {
        if(!oft[i].mptr)
            continue;
        if(running->fd[fd] == &oft[i])
            break;
    }
    if(i >= NOFT)
    {
        printf("ERROR: no %d fd in oft\n", fd);
        return -1;
    }

    // closing file
    oftp = running->fd[fd];
    running->fd[fd] = 0;
    oftp->refCount--;


    // not last user
    if(oftp->refCount > 0)
        return 0;

    // last user
    mip = oftp->mptr;
    iput(mip);

    oft[i].mptr = 0;

    return 0;
}

int my_lseek()
{
    int fd, position, i, ori;
    OFT *oftp;

    // check pathname and parameter are given
    if(pathname[0] == 0 || parameter[0] == 0)
    {
        printf("FORMAT ERROR: lseek (int)file_descriptor (int)position\n");
        return -1;
    }

    fd = atoi(pathname);
    position = atoi(parameter);

    // verify fd is within range
    if(fd < 0 || fd >= NFD)
    {
        printf("ERROR: fd is out of range\n");
        return -1;
    }

    // verify running->fd[fd] exists
    if(running->fd[fd] == 0)
    {
        printf("ERROR: running->fd[%d] doesn't exist\n", fd);
        return -1;
    }
    // get oft entry if it exists
    for(i=0; i < NOFT; i++)
    {
        if(!oft[i].mptr)
            continue;
        if(running->fd[fd] == &oft[i])
        {
            oftp = running->fd[fd];
            break;
        }
    }
    if(i >= NOFT)
    {
        printf("ERROR: no such fd in oft\n");
        return -1;
    }

    // make sure not to over run either end of the file
    if(position < 0 || position > oftp->mptr->INODE.i_size)
    {
        printf("ERROR: position value can only be between 0 - %d\n", oftp->mptr->INODE.i_size);
        return -1;
    }

    // change OFT entry's offset to position
    ori = oftp->offset;
    oftp->offset = position;
 
    // return original position
    return ori;
}

// print fd
int pfd()
{
    int i;
    OFT *oftp;

    printf("\n  fd    mode   offset   INODE\n");
    printf(" ----   ----   ------   -----\n");

    for(i=0; i < NFD; i++)
    {
        if(!running->fd[i])
            continue;
        
        oftp = running->fd[i];
        printf("  %2d     %s      %d     [%d, %d]  %x\n", i, filemode[oftp->mode],
            oftp->offset, oftp->mptr->dev, oftp->mptr->ino, oftp->mptr);
    }

    printf("----------- oft ----------------\n");
    for(i=0; i < NOFT; i++)
    {
        if(!oft[i].mptr)
            continue;
        
        oftp = &oft[i];
        printf("  %2d     %s      %d     [%d, %d]  %x\n", i, filemode[oftp->mode],
            oftp->offset, oftp->mptr->dev, oftp->mptr->ino, oftp->mptr);
    }
}

// write_file : check all writing conditions
int write_file()
{
    int fd, nbytes;
    char buf[BLKSIZE];
    OFT *oftp;

    // check pathname and parameter are given
    if(pathname[0] == 0 || parameter[0] == 0)
    {
        printf("FORMAT ERROR: write (int)fd (string)words\n");
        return -1;
    }

    fd = atoi(pathname);

    // string into buf
    strcpy(buf, parameter);
    
    // check fd is in range
    if(fd < 0 || fd >= NFD)
    {
        printf("ERROR: fd out of range\n");
        return -1;
    }

    // check fd exist
    if(running->fd[fd] == 0)
    {
        printf("ERROR: running->fd[%d] doesn't exist\n", fd);
        return -1;
    }
    
    oftp = running->fd[fd];

    // return if file not in W|RW|AP mode
    if(oftp->mode == 0)
    {
        printf("ERROR: fd is not open for W|RW|AP\n");
        return -1;
    }
    // call helper function
    return my_write(fd, buf, strlen(buf));
}

// zero out the block
int clr_blk(int buf[])
{
    int i, n;
    n = sizeof(buf) / sizeof(buf[0]);
    for(i=0; i < n; i++)
        buf[i] = 0;
}

// write helper function: write syscall()
int my_write(int fd, char buf[], int nbytes)
{
    MINODE *mip = running->fd[fd]->mptr;
    int offset = running->fd[fd]->offset;
    int oribytes = nbytes;
    int new, i;

    char wbuf[BLKSIZE];
    int lbk, start, remain, least;
    int blk, ibuf0[256], ibuf1[256];

    OFT *oftp = running->fd[fd];

    // write data into fd (lbk by lbk)
    while(nbytes > 0)
    {
        // compute logical block (lbk)
        lbk = oftp->offset / BLKSIZE;
        start = oftp->offset % BLKSIZE;
        remain = BLKSIZE - start;
        new = 0;
        printf("lbk = %d, start = %d, remain = %d\n", lbk, start, remain);

        // find data block to write to
        if(lbk < 12)    // direct block
        {
            if(mip->INODE.i_block[lbk] == 0)    // if block has no data
            {
                mip->INODE.i_block[lbk] = balloc(mip->dev); // allocate block
                printf("new allocated direct blk = %d\n", mip->INODE.i_block[lbk]);
            }
            blk = mip->INODE.i_block[lbk];
        }
        else if(lbk >= 12 && lbk < 256 + 12)    // indirect block
        {
            if(mip->INODE.i_block[12] == 0)     // if block 12 has no data
            {
                mip->INODE.i_block[12] = balloc(mip->dev);  // allocate a block
                new = 1;
            }
            get_block(dev, mip->INODE.i_block[12], ibuf0);
            
            if(new)     // zero out ibuf0 if it's new
                clr_blk(ibuf0);

            if(ibuf0[lbk - 12] == 0)    // if block has no data
            {
                ibuf0[lbk - 12] = balloc(mip->dev); // allocate block
                printf("new allocated indirect blk = %d\n", ibuf0[lbk - 12]);
            }

            blk = ibuf0[lbk - 12];
            put_block(dev, mip->INODE.i_block[12], ibuf0); // update the change
        }
        else    // double indirect block
        {
            if(mip->INODE.i_block[13] == 0) // if block 13 has no data
            {
                mip->INODE.i_block[13] = balloc(mip->dev);
                new = 1;
            }
            get_block(dev, mip->INODE.i_block[13], ibuf0);

            if(new)     // zero out blk if new
                clr_blk(ibuf0);
            new = 0;
            
            if(ibuf0[(lbk - (256 + 12)) / 256] == 0) // first indirect
            {
                ibuf0[(lbk - (256 + 12)) / 256] = balloc(mip->dev); 
                new = 1;
            }
            put_block(dev, mip->INODE.i_block[13], ibuf0);
            get_block(dev, ibuf0[(lbk - (256 + 12)) / 256], ibuf1);

            if(new)     // zero out blk if new
                clr_blk(ibuf0);

            if(ibuf1[(lbk - (256 + 12)) % 256] == 0) // second indirect
            {
                ibuf1[(lbk - (256 + 12)) % 256] = balloc(mip->dev);
                printf("new allocated double indirect blk = %d\n", ibuf1[(lbk - (256 + 12)) % 256]);
            }
            put_block(dev, ibuf0[(lbk - (256 + 12)) / 256], ibuf1);
            blk = ibuf1[(lbk - (256 + 12)) % 256];
        }
        
        // now write to block on disk
        get_block(mip->dev, blk, wbuf);

        char *cq = buf;             // read from cq
        char *cp = wbuf + start;    // write to cp

        if(remain > 0)  // if remain = 0, reach end of lbk
        {
            least = nbytes;
            if(remain < least)
                least = remain;
            
            memcpy(cp, cq, least);  // copy by the least amount
            nbytes -= least;
            remain -= least;
            oftp->offset += least;

            if(oftp->offset > mip->INODE.i_size)    // increase filesize as needed
                mip->INODE.i_size = oftp->offset;
        }

        put_block(mip->dev, blk, wbuf); // write to disk
        // loop if needed
    }

    mip->dirty = 1;
    printf("wrote %d bytes into fd = %d\n", oribytes, fd);
    iput(mip);
    return oribytes;
}

// read_file: check reading condition
int read_file()
{
    int nbyte, fd;
    char buf[BLKSIZE];
    OFT *oftp;

    // check pathname and parameter are given
    if(pathname[0] == 0 || parameter[0] == 0)
    {
        printf("FORMAT ERROR: read (int)file_descriptor (int)number_of_byte_to_read\n");
        return -1;
    }
    
    fd = atoi(pathname);
    nbyte = atoi(parameter);

    // check fd exists
    if(running->fd[fd] == 0)
    {
        printf("ERROR: running->fd[%d] doesn't exist\n", fd);
        return -1;
    }

    // check if it's open for R|RW
    oftp = running->fd[fd];
    if(oftp->mode % 2 == 1)
    {
        printf("ERROR: file wasn't open for R|RW\n");
        return -1;
    }
    return my_read(fd, buf, nbyte);
}

// read helper function like syscall read()
int my_read(int fd, char* buf, int nbyte)
{
    int lbk, offset, start, remain, avail, blk;
    int dbuf[256], ebuf[256];
    char readbuf[BLKSIZE];
    MINODE *mip;
    int count = 0, least;
    char *cp, *cq;

    offset = running->fd[fd]->offset;
    mip = iget(running->fd[fd]->mptr->dev, running->fd[fd]->mptr->ino);

    // byte avail for read
    avail = running->fd[fd]->mptr->INODE.i_size - offset;

    while(nbyte > 0 && avail > 0)
    {
        // current byte position
        lbk = offset / BLKSIZE;
        // byte to start reading
        start = offset % BLKSIZE;

        if(lbk < 12)    // direct block
            blk = mip->INODE.i_block[lbk];
        else if(lbk >= 12 && lbk < 256 + 12)    // indirect block
        {
            get_block(mip->dev, mip->INODE.i_block[12], dbuf);
            blk = dbuf[lbk-12];
        }
        else    // double indirect block
        {
            get_block(mip->dev, mip->INODE.i_block[13], dbuf);
            get_block(mip->dev, dbuf[(lbk - (156 + 12)) / 256], ebuf);
            blk = ebuf[(lbk-156-12) % 256];
        }

        // get data block into readbuf
        get_block(mip->dev, blk, readbuf);

        cp = readbuf + start;   // real data buffer
        cq = buf;               // buffer for read
        // remaining byte on lbk
        remain = BLKSIZE - start;

        if(remain > 0)
        {
            // get the least of avail, remain and nbyte
            least = nbyte;
            if(least > avail)
                least = avail;
            if(least > remain)
                least = remain;

            memcpy(cq, cp, least);
            running->fd[fd]->offset += least;
            count += least;
            avail -= least;
            nbyte -= least;
        }
    }
    
    //printf("read %d char\n", count);
    iput(mip);
    return count;
}

// read and print content of file
int my_cat()
{
    char mybuf[BLKSIZE];
    int i, k, fd;

    // check pathname is given
    if(pathname[0] == 0)
    {
        printf("FORMAT ERROR: cat (string)pathname\n");
        return -1;
    }

    // put R mode in parameter
    strcpy(parameter, "R");

    // open file for read
    if((fd = open_file()) < 0)
        return -1;
    
    // reading loop
    while((i = my_read(fd, mybuf, BLKSIZE)) > 0)
    {
        mybuf[i] = 0;   // null char at end
        // spit out each char from mybuf
        for(k=0; k < i; k++)
            putchar(mybuf[k]);
    }
    printf("\n");

    // preparation to close file
    sprintf(pathname, "%d", fd);
    parameter[0] = 0;

    // close file
    if(close_file() < 0)
        return -1;
}

// copy file to another file
int my_cp()
{
    char src[256], dest[256], buf[BLKSIZE];
    char parent[256];
    int fd, gd, i, ino;
    MINODE *pip, *mip;

    // check pathname is given
    if(pathname[0] == 0 || parameter[0]==0)
    {
        printf("FORMAT ERROR: cp (string)source (string)destination\n");
        return -1;
    }

    strcpy(src, pathname);
    strcpy(dest, parameter);
    printf("src = %s, pathname = %s\n", src, pathname);
    printf("dest = %s, parameter = %s\n", dest, parameter);
    
    // open src for R
    strcpy(parameter, "R");
    if((fd = open_file()) < 0)
    {   
        printf("ERROR: failed to open source file\n");
        return -1;
    }
    pfd();
    strcpy(pathname, dest);
    parameter[0] = 0;

    // creat file if not already exist
    printf("create file\n");
    if(creat_file() < 0)
    {
        if((ino = getino(pathname)) == 0)
        {
            printf("creat %s failed\n", dest);
            return -1;
        }
    }

    strcpy(parameter, "W");

    // open file
    pfd();
    if((gd = open_file()) < 0)
    {
        pfd();
        printf("cp ERROR: can't open file\n");
        return -1;
    }

    // transfer data from src to dest
    while(i = my_read(fd, buf, BLKSIZE))
        my_write(gd, buf, i);
    
    // close src
    sprintf(pathname, "%d", fd);
    close_file();

    // close dest
    sprintf(pathname, "%d", gd);
    close_file();

    return 0;
}




