/*********** type.h file ************/
#define NPROC    9          // number of PROCs
#define SSIZE 1024          // stack size = 4KB
#define BSIZE    8          // buffer size = 8

// PROC status
#define FREE     0          
#define READY    1
#define SLEEP    2
#define ZOMBIE   3
#define BLOCK    4

typedef struct proc{
    struct proc *next;      // next proc pointer       
    int  *ksp;              // saved sp: at byte offset 4 

    int   pid;              // process ID
    int   ppid;             // parent process pid 
    int   status;           // PROC status=FREE|READY, etc. 
    int   priority;         // scheduling priority 

    int   event;            // event value to sleep on
    int   exitCode;         // exit value

    struct proc *child;     // first child PROC pointer       
    struct proc *sibling;   // sibling PROC pointer  
    struct proc *parent;    // parent PROC pointer       

    int   kstack[1024];     // process stack                 
}PROC;


typedef struct semaphore{
    int value;      // initial val of semaphore
    PROC *queue;    // Queue of blocked processes
}SEMAPHORE;


typedef struct buffer{
  char buf[BSIZE];
  int head, tail;
  struct semaphore data, room;
  struct semaphore mutex;
}BUFFER;