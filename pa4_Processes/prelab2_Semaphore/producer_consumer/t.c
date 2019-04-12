/*********** t.c file of A Multitasking System *********/
#include <stdio.h>
#include "string.h"
#include "type.h"

PROC proc[NPROC];      // NPROC PROCs
PROC *freeList;        // freeList of PROCs 
PROC *readyQueue;      // priority queue of READY procs
PROC *running;         // current running proc pointer

PROC *sleepList;       // list of SLEEP procs
BUFFER buffer;

#include "queue.c"     // include queue.c file
#include "wait.c"      // include wait.c file

/*******************************************************
  kfork() creates a child process; returns child pid.
  When scheduled to run, child PROC resumes to body();
********************************************************/

// initialize the MT system; create P0 as initial running process
int init() 
{
  int i;
  PROC *p;
  for (i=0; i<NPROC; i++){ // initialize PROCs
    p = &proc[i];
    p->pid = i;            // PID = 0 to NPROC-1  
    p->status = FREE;
    p->priority = 0;      
    p->next = p+1;
    p->parent = p;        // set parent to itself 
    p->child = NULL;      // child, and sibling to NULL
    p->sibling = NULL;
    p->exitCode = 0;      // exitCode = 0
    p->event = 0;
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

// initialize buffer and semaphore
int buffer_init()
{
  BUFFER *p = &buffer;
  p->head = p->tail = 0;
  p->data.value = 0;      p->data.queue = 0;
  p->room.value = BSIZE;  p->room.queue = 0;
  p->mutex.value = 1;     p->mutex.queue = 0;
}


int kfork(int func)
{
  int  i;
  PROC *p = dequeue(&freeList);
  PROC *temp = NULL;
  if (!p){
     printf("no more proc\n");
     return(-1);
  }
  /* initialize the new proc and its stack */
  p->status = READY;
  p->priority = 1;       // ALL PROCs priority=1, except P0
  p->ppid = running->pid;

    // insert child
  if(!running->child)
    running->child = p;
  else{
    temp = running->child;

    while(temp->sibling)
      temp = temp->sibling;
    
    temp->sibling = p;
  }
  
  /************ new task initial stack contents ************
   kstack contains: |retPC|eax|ebx|ecx|edx|ebp|esi|edi|eflag|
                      -1   -2  -3  -4  -5  -6  -7  -8   -9
  **********************************************************/
  for (i=1; i<10; i++)               // zero out kstack cells
      p->kstack[SSIZE - i] = 0;
  p->kstack[SSIZE-1] = (int)func;    // retPC -> func()
  p->ksp = &(p->kstack[SSIZE - 9]);  // PROC.ksp -> saved eflag 
  enqueue(&readyQueue, p);           // enter p into readyQueue
  return p->pid;
}

/*************** main() function ***************/
int main()
{
   printf("Welcome to the MT Multitasking System\n");
   init();    // initialize system; create and run P0
   buffer_init(); //initialize buffer and semaphore
   kfork(producer);   // kfork producer
   kfork(consumer);   // kfork consumer 
   while(1){
     printf("P0: switch process\n");
     while (readyQueue == 0);
         tswitch();
   }
}

/*********** scheduler *************/
int scheduler()
{ 
  printf("proc %d in scheduler()\n", running->pid);
  if (running->status == READY)
     enqueue(&readyQueue, running);
  printList("readyQueue", readyQueue);
  running = dequeue(&readyQueue);
  printf("next running = %d\n", running->pid);  
}
