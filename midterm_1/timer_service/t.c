#include "type.h"
#include "string.c"

PROC proc[NPROC];      // NPROC PROCs
TIMER *timerp[4];
TQE telement[NPROC];   // NPROC timer elements
PROC *freeList;        // freeList of PROCs 
PROC *readyQueue;      // priority queue of READY procs
PROC *running;         // current running proc pointer

PROC *sleepList;       // list of SLEEP procs
TQE *tq;               // timer queue
int procsize = sizeof(PROC);

#define printf kprintf
#define gets kgets

#include "kbd.c"
#include "vid.c"
#include "exceptions.c"

#include "queue.c"
#include "wait.c"      // include wait.c file
#include "tqe.c"
#include "timer.c"

/*******************************************************
  kfork() creates a child process; returns child pid.
  When scheduled to run, child PROC resumes to body();
********************************************************/
int body(), tswitch(), do_sleep(), do_wakeup(), do_exit(), do_switch();
int do_kfork(), do_wait();
int scheduler();

int kprintf(char *fmt, ...);

void copy_vectors(void) {
    extern u32 vectors_start;
    extern u32 vectors_end;
    u32 *vectors_src = &vectors_start;
    u32 *vectors_dst = (u32 *)0;
    while(vectors_src < &vectors_end)
       *vectors_dst++ = *vectors_src++;
}

void IRQ_handler()
{
    int vicstatus, sicstatus;
    int ustatus, kstatus;

    // read VIC SIV status registers to find out which interrupt
    vicstatus = VIC_STATUS;
    sicstatus = SIC_STATUS;

    // VIC status bit 4 => timer0, timer1
    if (vicstatus & (1 << 4)){
      if(*(timerp[0]->base+TVALUE)!=0)  // timer0
        timer_handler(0);
      if(*(timerp[1]->base+TVALUE)!=0)  // timer1
        timer_handler(1);
    }

    // VIC status bit 5 => timer2,timer3
    if (vicstatus & (1 << 5)){
      if(*(timerp[2]->base+TVALUE)!=0)  // timer2
        timer_handler(2);
      if(*(timerp[3]->base+TVALUE)!=0)  // timer3
        timer_handler(3);
    }

    if (vicstatus & 0x80000000){ // SIC interrupts=bit_31=>KBD at bit 3 
       if (sicstatus & 0x08){
          kbd_handler();
       }
    }
}

// initialize the MT system; create P0 as initial running process
int init() 
{
  int i;
  PROC *p;
  TQE *tp;
  for (i=0; i<NPROC; i++){ // initialize PROCs
    p = &proc[i];
    tp = &telement[i];
    // proc init
    p->pid = i;            // PID = 0 to NPROC-1  
    p->status = FREE;
    p->priority = 0;      
    p->next = p+1;
    p->child = 0;
    p->parent = 0;
    p->sibling = 0;
    p->exitCode = 0;
    p->event = 0;
    // timer init
    tp->pid = i;
    tp->time = 0;
    tp->next = 0;
  }
  proc[NPROC-1].next = 0;  
  freeList = &proc[0];     // all PROCs in freeList     
  readyQueue = 0;          // readyQueue = empty

  sleepList = 0;           // sleepList = empty
  
  // create P0 as the initial running process
  p = running = dequeue(&freeList); // use proc[0] 
  p->status = READY;
  p->priority = 0;
  p->ppid = 0;             // P0 is its own parent
  
  printList("freeList", freeList);
  printf("init complete: P0 running\n"); 
}

int menu()
{
  printf("*****************************************\n");
  printf(" ps fork switch exit sleep wakeup wait t\n");
  printf("*****************************************\n");
}

char *status[ ] = {"FREE", "READY", "SLEEP", "ZOMBIE"};

int do_ps()
{
  int i;
  PROC *p;
  printf("PID  PPID  status\n");
  printf("---  ----  ------\n");
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    printf(" %d    %d    ", p->pid, p->ppid);
    if (p == running)
      printf("RUNNING\n");
    else
      printf("%s\n", status[p->status]);
  }
}
    
int body()   // process body function
{
  int c;
  char cmd[64];
  PROC *temp = 0;
  printf("proc %d starts from body()\n", running->pid);
  while(1){
    printf("***************************************\n");
    printf("proc %d running: parent=%d\n", running->pid,running->ppid);
    printf("childlist = ");
    if(!running->child)
      printf("NULL\n");
    else{
      temp = running->child;
      printf("[%d %s]", temp->pid, status[temp->status]);

      while(temp->sibling){
        printf("->[%d %s]", temp->sibling->pid, status[temp->sibling->status]);
        temp = temp->sibling;
      }

      printf("->NULL\n");
    }
    printList("readyQueue", readyQueue);
    printSleepList(sleepList);
    menu();
    printf("enter a command : ");
    gets(cmd);
    
    if (strcmp(cmd, "ps")==0)
      do_ps();
    if (strcmp(cmd, "fork")==0)
      do_kfork();
    if (strcmp(cmd, "switch")==0)
      do_switch();
    if (strcmp(cmd, "exit")==0)
      do_exit();
    if (strcmp(cmd, "sleep")==0)
      do_sleep();
    if (strcmp(cmd, "wakeup")==0)
      do_wakeup();
    if (strcmp(cmd, "wait")==0)
      do_wait();
    if (strcmp(cmd, "t")==0)
      t();
  }
}

int kfork()
{
  int i;
  PROC *p = dequeue(&freeList);
  PROC *temp = 0;
  if (p==0){
    kprintf("kfork failed\n");
    return -1;
  }
  p->ppid = running->pid;
  p->parent = running;
  p->status = READY;
  p->priority = 1;

  // insert child
  if(!running->child)
    running->child = p;
  else{
    temp = running->child;

    while(temp->sibling)
      temp = temp->sibling;

    temp->sibling = p;
  }
  
  // set kstack to resume to body
  // stack = r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r14
  //         1  2  3  4  5  6  7  8  9  10 11  12  13  14
  for (i=1; i<15; i++)
      p->kstack[SSIZE-i] = 0;
  p->kstack[SSIZE-1] = (int)body;  // in dec reg=address ORDER !!!
  p->ksp = &(p->kstack[SSIZE-14]);
 
  enqueue(&readyQueue, p);
  return p->pid;
}

int do_kfork()
{
   int child = kfork();
   if (child < 0)
      printf("kfork failed\n");
   else{
      printf("proc %d kforked a child = %d\n", running->pid, child); 
      printList("readyQueue", readyQueue);
   }
   return child;
}

int do_switch()
{
   tswitch();
}

int do_exit()
{
  if(running->pid == 1)
    printf("P1 never die\n");
  else
    kexit(running->pid);  // exit with own PID value 
}

int do_sleep()
{
  
  int event;
  printf("enter an event value to sleep on : ");
  event = geti();
  ksleep(event);
}

int do_wakeup()
{
  int event;
  printf("enter an event value to wakeup with : ");
  event = geti();
  kwakeup(event);
}

int do_wait()
{
  int status = 0;   // exit status
  int pid = kwait(&status);
}

int main()
{ 
  int i; 
  char line[128]; 
  u8 kbdstatus, key, scode;
  KBD *kp = &kbd;
  color = WHITE;
  row = col = 0; 

  fbuf_init();
  kprintf("Welcome to Wanix in ARM\n");
  kbd_init();
  
  // enable timer interrupts
  VIC_INTENABLE |= (1<<4);
  VIC_INTENABLE |= (1<<5);
  /* enable SIC interrupts */
  VIC_INTENABLE |= (1<<31); // SIC to VIC's IRQ31
  /* enable KBD IRQ */
  SIC_INTENABLE = (1<<3); // KBD int=bit3 on SIC
  SIC_ENSET = (1<<3);  // KBD int=3 on SIC
  *(kp->base+KCNTL) = 0x12;

  timer_init();
  init();

  timerp[0] = &timer[0];
  timer_start(0);

  printQ(readyQueue);
  kfork();   // kfork P1 into readyQueue  

  unlock();
  while(1){
    if (readyQueue)
      tswitch();
  }
}

/*********** scheduler *************/
int scheduler()
{ 
  //printf("proc %d in scheduler()\n", running->pid);
  if (running->status == READY)
     enqueue(&readyQueue, running);
  //printList("readyQueue", readyQueue);
  running = dequeue(&readyQueue);
  //printf("next running = %d\n", running->pid);  
}


