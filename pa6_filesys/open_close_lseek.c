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
    char md[5];
    MINODE *mip;
    OFT *oftp;

    // check pathname is given
    if(pathname[0] == 0 || parameter[0] == 0)
    {
        printf("FORMAT ERROR: open [pathname] [mode]\n");
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

    /*if(!strcmp(parameter, "R"))
        mode = 0;
    else if(!strcmp(parameter, "W"))
        mode = 1;
    else if (!strcmp(parameter, "RW"))
        mode = 2;
    else if (!strcmp(parameter, "AP"))
        mode = 3;
    else  
        mode = atoi(parameter);*/

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
        printf("FORMAT ERROR: close [file_descripter]\n");
        printf("see file descripter by using command: pfd\n");
        return -1;
    }

    fd = atoi(pathname);
    printf("fd = %d\n", fd);
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
        printf("  %2d     %s      %d     [%d, %d]\n", i, filemode[oftp->mode],
            oftp->offset, oftp->mptr->dev, oftp->mptr->ino);
    }

    for(i=0; i < NOFT; i++)
    {
        if(!oft[i].mptr)
            continue;
        
        oftp = &oft[i];
        printf("  %2d     %s      %d     [%d, %d]\n", i, filemode[oftp->mode],
            oftp->offset, oftp->mptr->dev, oftp->mptr->ino);
    }
}




