/*************** type.h file ************************/
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;   

#define FREE        0
#define READY       1

#define BLKSIZE  1024
#define NMINODE   256
#define NOFT      256
#define NFD        64
#define NPROC      16

typedef struct minode{
    INODE INODE;
    int dev, ino;
    int refCount;
    int dirty;
    int mounted;
    struct mntable *mptr;
}MINODE;

typedef struct oft{
    int  mode;
    int  refCount;
    MINODE *mptr;
    int  offset;
}OFT;

typedef struct proc{
    struct proc *next;
    int          pid;
    int          ppid;
    int          status;
    int          uid, gid;
    MINODE      *cwd;
    OFT         *fd[NFD];
}PROC;