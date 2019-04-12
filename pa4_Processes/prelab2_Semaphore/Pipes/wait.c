int tswitch();

int ksleep(int event)
{
  printf("proc %d going to sleep on event=%d\n", running->pid, event);

  running->event = event;
  running->status = SLEEP;
  enqueue(&sleepList, running);
  printList("sleepList", sleepList);
  tswitch();
}

int kwakeup(int event)
{
  PROC *temp, *p;
  temp = 0;
  printList("sleepList", sleepList);

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
  printList("sleepList", sleepList);
}

int kexit(int exitValue)
{
  PROC *temp = running;
  PROC *p1 = &proc[1];

  printf("proc %d in kexit(), value=%d\n", running->pid, exitValue);
  running->exitCode = exitValue;
  running->status = ZOMBIE;
  // move children to p1
  if(temp->child){
    temp = temp->child;
    // change parent to p1
    temp->ppid = 1;
    temp->parent = &proc[1];
    
    // insert to p1 child
    if(!p1->child)
      p1->child = temp;
    else{
      p1 = p1->child;
      while(p1->sibling)
        p1 = p1->sibling;
      p1->sibling = temp;
    }

    // change all attached sibling's parent
    while(temp->sibling){
      temp->sibling->ppid = 1;
      temp->sibling->parent = &proc[1];
      temp = temp->sibling;
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
  PROC *prev = NULL;

  // return if caller has no child
  if(!p->child)
    return -1;
  
  // if caller has children
  while(1){
    // search for zombie child
    p = running->child;
    while(p){
      if(p->status == ZOMBIE){
        *status = p->exitCode;
        // take out from child tree
        if(p->pid == running->child->pid)
          running->child = p->sibling;
        else
          prev->sibling = p->sibling;
        // reset attributes
        p->status = FREE;
        p->ppid = 0;
        p->priority = 0;
        p->parent = NULL;     
        p->child = NULL;
        p->sibling = NULL;
        p->exitCode = 0;      
        p->event = 0;
        // add to freelist
        enqueue(&freeList, p);
        return p->pid;
      }
      prev = p;
      p = p->sibling;
    }
    ksleep(running);
  }
}
  
