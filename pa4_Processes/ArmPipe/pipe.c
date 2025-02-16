int show_pipe()
{
    PIPE *p = &pipe;
    int i;
    printf("----------------------------------------\n");
    printf("room=%d data=%d buf=", p->room, p->data);
    for (i=0; i<p->data; i++)
        kputc(p->buf[p->tail+i]);
    printf("\n");
    printf("----------------------------------------\n");
}
int kpipe()
{
    int i;
    PIPE *p = &pipe;
    p->head = p->tail = 0;
    p->data = 0; p->room = PSIZE;
    p->writer = 0;
    p->reader = 0;
}

int read_pipe(PIPE *p, char *buf, int n)
{
    int ret;
    char c;

    if (n<=0)
        return 0;
    
    if(p->data == 0 && p->writer == 0)
        return 0;
    
    show_pipe();

    while(n){
        printf("reader %d reading pipe\n", running->pid);
        ret = 0;
        while(p->data){
            *buf = p->buf[p->tail++];
            p->tail %= PSIZE;
            buf++;  ret++; 
            p->data--; p->room++; n--;
            if (n <= 0)
                break;
        }
        show_pipe();
        if (ret){   /* has read something */
            kwakeup(&p->room);
            return ret;
        }
        // pipe has no data
        printf("reader %d sleep for data\n", running->pid);
        kwakeup(&p->room);
        ksleep(&p->data);
        continue;
    }
}

int write_pipe(PIPE *p, char *buf, int n)
{
    char c;
    int ret = 0;

    show_pipe();
    while (n){
        printf("writer %d writing pipe\n", running->pid);
        while (p->room){
            p->buf[p->head++] = *buf; 
            p->head  %= PSIZE;
            buf++;  ret++; 
            p->data++; p->room--; n--;
            if (n<=0){
                show_pipe();
	        kwakeup(&p->data);
	        return ret;
            }
        }
        show_pipe();
        printf("writer %d sleep for room\n", running->pid);
        kwakeup(&p->data);
        ksleep(&p->room);
    }
}
 
int pipe_reader()
{
    char line[128];
    int nbytes, n;
    KBD *kp = &kbd;
    PIPE *p = &pipe;
    printf("proc %d as pipe reader\n", running->pid);
 
    while(1){
        printf("input nbytes to read (0 to kill): " );
        nbytes = geti();
        if(nbytes <= 0){
            p->reader--;
            do_exit();
        }

        n = read_pipe(p, line, nbytes);
        if(nbytes > 0 && n == 0){
            n = 0;
            strcpy(line, "");
            /*printf("nbytes = %d, n = %d\n", nbytes, n);
            p->reader--;
            do_exit();*/
        }
        line[n] = 0;
        printf("Read n=%d bytes : line=%s\n", n, line);
    }
}


int pipe_writer()
{
    char line[128];
    int nbytes, n;
    PIPE *p = &pipe;
    printf("proc %d as pipe writer\n", running->pid);

    while(1){

        printf("input a string to write (directly press enter to kill): " );

        kgets(line);

        // if broken pipe
        if(p->reader == 0){
            printf("BROKEN PIPE... exit\n");
            p->writer--;
            do_exit();
        }

        if (strcmp(line, "")==0){
            p->writer--;
            do_exit();
        }

        nbytes = strlen(line);
        printf("nbytes=%d buf=%s\n", nbytes, line);
        n = write_pipe(p, line, nbytes);

        printf("wrote n=%d bytes\n", n);
    }
}
