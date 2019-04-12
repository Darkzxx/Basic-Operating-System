//int tswitch();

int ksleep(int event)
{
  int sr = int_off();
  printf("proc %d going to sleep on event=%d\n", running->pid, event);

  running->event = event;
  running->status = SLEEP;
  enqueue(&sleepList, running);
  printSleepList(sleepList);
  tswitch();
  int_on(sr);
}

int kwakeup(int event)
{
  PROC *temp, *p;
  temp = 0;
  int sr = int_off();
  
  printSleepList(sleepList);
  printf("wakeup event =  %d\n", event);

  while (p = dequeue(&sleepList)){
    if (p->event == event){
	    printf("wakeup %d\n", p->pid);
	    p->status = READY;
	    enqueue(&readyQueue, p);
    }
    else{
	    enqueue(&temp, p);
    }
  }
  sleepList = temp;
  printSleepList(sleepList);
  printQ(readyQueue);
  int_on(sr);
}

int kexit(int exitValue)
{
  int i, temp;
  
  if(running->pid < 2){
    temp = color;
    color = RED;
    kprintf("Proc %d exit error: Proc 1 can't exit\n", running->pid);
    color = temp;
    return -1;
  }

  printf("proc %d in kexit(), value=%d\n", running->pid, exitValue);
  running->exitCode = exitValue;
  running->status = ZOMBIE;

  // move children to p1
  for(i = 0; i < NPROC; i++){
    if(proc[i].ppid == running->pid){
      proc[i].ppid = proc[1].pid;
      proc[i].parent = &proc[1];
    }
  }
  
  printf("exit parent address = %d\n", &proc[running->ppid]);
  kwakeup(&proc[running->ppid]);
  tswitch();
}

int kwait(int *status)
{
  int i = 0;
  PROC *p = running;
  PROC *prev = 0;

  // wait for zombie child and free it when found
  while(1){
    for(i = 0; i < NPROC; i++){
      if(proc[i].ppid == running->pid && proc[i].status == ZOMBIE){
        *status = proc[i].exitCode;
        proc[i].status = FREE;
        proc[i].ppid = 0;
        proc[i].priority = 0;
        proc[i].parent = 0;     
        proc[i].exitCode = 0;      
        proc[i].event = 0;

        enqueue(&freeList, &proc[i]);
        return proc[i].pid;
      }
    }
    ksleep(running);
  }
}

  
