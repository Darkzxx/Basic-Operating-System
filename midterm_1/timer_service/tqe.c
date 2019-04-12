// t function : create tqe and insert to tq
int t()
{
    TQE *tp = &telement[running->pid];
    int sec = 0;

    kprintf("sleep P%d for (sec): ", running->pid);
    sec = geti();

    if(sec <= 0)
        return 0;

    tp->time = sec;
    enqueueT(tp);
    ksleep(&telement[tp->pid]);
}