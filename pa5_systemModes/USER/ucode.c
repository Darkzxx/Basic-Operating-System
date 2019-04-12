typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned int    u32;

#include "string.c"
#include "uio.c"

int ubody(char *name)
{
  int pid, ppid;
  char line[64];
  u32 mode,  *up;

  mode = getcsr();
  mode = mode & 0x1F;
  printf("CPU mode=%x\n", mode);
  pid = getpid();
  ppid = getppid();

  while(1){
    if(!strcmp(name, "one"))
      printf("This is process #%d in Umode at %x parent=%d\n", pid, getPA(),ppid);
    else
      printf("DAS IST PROZESS #%d IM USER_MODUS at %x ELTERN=%d\n", pid, getPA(),ppid);
    umenu();
    if(!strcmp(name, "one"))
      printf("input a command : ");
    else
      printf("BEFEHL : ");
    ugetline(line); 
    uprintf("\n"); 
 
    if (strcmp(line, "getpid")==0)
      ugetpid();
    if (strcmp(line, "getppid")==0)
      ugetppid();
    if (strcmp(line, "ps")==0)
      ups();
    if (strcmp(line, "chname")==0)
      uchname();
    if (strcmp(line, "switch")==0)
      uswitch();
    if (strcmp(line, "sleep")==0)
      usleep();
    if (strcmp(line, "wakeup")==0)
      uwakeup();
    if (strcmp(line, "wait")==0)
      uwait();
    if (strcmp(line, "exit")==0)
      uexit();
    if (strcmp(line, "kfork")==0)
      ukfork();
    if (strcmp(line, "fork")==0)
      printf("return pid = %d, p->pid = %d\n", ufork(), getpid());
    if (strcmp(line, "exec")==0)
      uexec();
  }
}

int umenu()
{
  uprintf("-----------------------------------------------------------------------\n");
  uprintf("getpid getppid ps chname kfork switch sleep wakeup wait fork exec exit\n");
  uprintf("-----------------------------------------------------------------------\n");
}

int getpid()
{
  int pid;
  pid = syscall(0,0,0,0);
  return pid;
}    

int getppid()
{ 
  return syscall(1,0,0,0);
}

int ugetpid()
{
  int pid = getpid();
  uprintf("pid = %d\n", pid);
}

int ugetppid()
{
  int ppid = getppid();
  uprintf("ppid = %d\n", ppid);
}

int ups()
{
  return syscall(2,0,0,0);
}

int uchname()
{
  char s[32];
  uprintf("input a name string : ");
  ugetline(s);
  printf("\n");
  return syscall(3,s,0,0);
}

int uswitch()
{
  return syscall(4,0,0,0);
}

int usleep()
{
  int i;
  uprintf("input a number event to sleep on : ");
  i = geti();
  printf("\n");
  return syscall(5,i,0,0);
}

int uwakeup()
{
  int i;
  uprintf("input a number event to wakeup : ");
  i = geti();
  printf("\n");
  return syscall(6,i,0,0);
}

int uwait()
{
  return syscall(7,0,0,0);
}

int uexit()
{
  return syscall(8,0,0,0);
}

int ukfork()
{
  return syscall(9,0,0,0);
}

int ufork()
{
  return syscall(10,0,0,0);
}

int uexec()
{
  char s[128];
  uprintf("enter a command string : ");
  ugetline(s);
  printf("\n");
  return syscall(11,s,0,0);
}

int ugetc()
{
  return syscall(90,0,0,0);
}

int uputc(char c)
{
  return syscall(91,c,0,0);
}

int getPA()
{
  return syscall(92,0,0,0);
}


