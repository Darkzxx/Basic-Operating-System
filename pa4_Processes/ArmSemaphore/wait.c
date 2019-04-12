int tswitch();

int ksleep(int event)
{
  int sr = int_off();
  printf("proc %d going to sleep on event=%d\n", running->pid, event);

  running->event = event;
  running->status = SLEEP;
  enqueue(&sleepList, running);
  printList("sleepList", sleepList);
  tswitch();
  int_on(sr);
}

int kwakeup(int event)
{
  PROC *temp, *p;
  temp = 0;
  int sr = int_off();
  
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
  int_on(sr);
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
  PROC *prev = 0;

  // return if caller has no child
  if(!running->child)
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
        p->parent = 0;     
        p->child = 0;
        p->sibling = 0;
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

int show_buffer()
{
  BUFFER *b = &buffer;
  int i;
  printf("------------ BF -----------------\n");
  printf("room=%d data=%d buf=", b->room.value, b->data.value);
  for (i=0; i<b->data.value; i++)
    kputc(b->buf[b->tail+i]);
  printf("\n");
  printf("----------------------------------\n");
}

int P(SEMAPHORE *s)
{
  s->value--;
  if(s->value < 0){
    printf("proc %d BLOCK\n", running->pid);
    //show_buffer();
    running->status = BLOCK;
    enqueue(&s->queue, running);
    tswitch();
  }
}

int V(SEMAPHORE *s)
{
  PROC *p = 0;
  s->value++;
  if(s->value <= 0){
    p = dequeue(&s->queue);
    p->status = READY;
    enqueue(&readyQueue, p);
    printf("V-up %d\n", p->pid);
    //show_buffer();
  }
}


int produce(char c)
{
  BUFFER *p = &buffer;
  P(&p->room);
  P(&p->mutex);
  p->buf[p->head++] = c;
  p->head %= BSIZE;
  V(&p->mutex);
  V(&p->data);
}

int consume()
{
  int c;
  BUFFER *p = &buffer;
  P(&p->data);
  P(&p->mutex);
  c = p->buf[p->tail++];
  p->tail %= BSIZE;
  V(&p->mutex);
  V(&p->room);
  return c;
}
 
int consumer()
{
  char line[128];
  KBD *kp = &kbd;
  int nbytes, n, i;

  printf("proc %d as consumer\n", running->pid);
 
  while(1){
    printf("input nbytes to read : " );
    P(&kp->line);
    nbytes = geti();
    //show_buffer();
    for (i=0; i<nbytes; i++){
       line[i] = consume();
       printf("%c", line[i]);
    }
    line[i] = 0;
    printf("\n");
    show_buffer();
    printf("consumer %d got n=%d bytes : line=%s\n", running->pid, nbytes, line);
  }
}

int producer()
{
  char line[128];
  KBD *kp = &kbd;
  int nbytes, n, i;

  printf("proc %d as producer\n", running->pid);

  while(1){
    printf("input a string to produce : " );
    
    P(&kp->line);
    kgets(line);
    nbytes = strlen(line);

    printf("nbytes=%d line=%s\n", nbytes, line);
    //show_buffer();
    for (i=0; i<nbytes; i++){
      produce(line[i]);
    }
    show_buffer();
    printf("producer %d put n=%d bytes\n", running->pid, nbytes);
  }
}
