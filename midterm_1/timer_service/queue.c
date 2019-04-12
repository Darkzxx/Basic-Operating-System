/********************************************************************
Copyright 2010-2017 K.C. Wang, <kwang@eecs.wsu.edu>
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************/

// queue.c file

int enqueue(PROC **queue, PROC *p)
{
  PROC *q  = *queue;
  if (q==0){
    *queue = p;
    p->next = 0;
    return;
  }
  if ((*queue)->priority < p->priority){
    p->next = *queue;
    *queue = p;
    return;
  }
  while (q->next && p->priority <= q->next->priority){
    q = q->next;
  }
  p->next = q->next;
  q->next = p;
}

PROC *dequeue(PROC **queue)
{
  PROC *p = *queue;
  if (p)
    *queue = p->next;
  return p;
}

int printQ(PROC *p)
{
  kprintf("readyQueue = ");
  while(p){
    kprintf("[%d%d]->", p->pid,p->priority);
    p = p->next;
  }
  kprintf("NULL\n");
}

int printSleepList(PROC *p)
{
  kprintf("sleepList   = ");
   while(p){
     kprintf("[%devent=%d]->", p->pid,p->event);
     p = p->next;
  }
  kprintf("NULL\n"); 
}

int printList(char *name, PROC *p)
{
  kprintf("%s = ", name);
   while(p){
     kprintf("[%d%d]->", p->pid, p->priority);
     p = p->next;
  }
  kprintf("NULL\n"); 
}

//******** for TQE **********
int enqueueT(TQE *tp)
{
  TQE *q = tq;

  kprintf("time = %d\n", tp->time);
  // queue empty
  if(q == 0){
    tq = tp;
    return;
  }

  // insert at head if tp time <= head time
  if(tp->time <= tq->time){
    tq->time -= tp->time;
    tp->next = tq;
    tq = tp;
    return;
  }

  // traverse til tp time <= q->time or til null
  while(q->next && tp->time > q->next->time){
    printf("q->nexttime = %d\n",q->next->time);
    kprintf("tp->time = %d\n", tp->time);
    tp->time -= q->time;  // substract out time
    q = q->next;
  }
  tp->time -= q->time;
  tp->next = q->next;
  q->next = tp;
}

TQE *dequeueT()
{
  TQE *q = tq;
  if(q)
    tq = q->next;
  return q;
}





