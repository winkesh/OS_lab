#include "types.h"
#include "stat.h"
#include "user.h"

#define NCHILD 4
#define SAMPLES 10

static void
cpu_job(void)
{
  volatile int x = 0;
  for(;;)
    x++;
}

static void
io_job(void)
{
  volatile int x = 0;
  int i;
  for(;;){
    for(i = 0; i < 20000; i++)
      x++;
    sleep(1);
  }
}

int
main(void)
{
  char *kind[NCHILD] = {"CPU", "I/O", "CPU", "I/O"};
  int pids[NCHILD];
  int last[NCHILD];
  int i, s, pid, ok = 1;

  printf(1, "mlfqtest: 2 CPU-bound children (infinite loops) and 2 I/O-bound children\n");
  printf(1, "(tiny compute, then sleep(1), forever). Time slices: queue 0 = 1 tick,\n");
  printf(1, "queue 1 = 2 ticks, queue 2 = 4 ticks. The parent samples every queue level.\n\n");

  for(i = 0; i < NCHILD; i++){
    pid = fork();
    if(pid < 0){
      printf(1, "fork failed\n");
      exit();
    }
    if(pid == 0){
      if(kind[i][0] == 'C')
        cpu_job();
      io_job();
    }
    pids[i] = pid;
    printf(1, "parent: pid %d is %s-bound\n", pid, kind[i]);
  }

  printf(1, "\ntick   ");
  for(i = 0; i < NCHILD; i++)
    printf(1, "pid%d(%s) ", pids[i], kind[i]);
  printf(1, "\n");
  for(s = 0; s < SAMPLES; s++){
    sleep(10);
    printf(1, "%d    ", uptime());
    for(i = 0; i < NCHILD; i++)
      printf(1, "  queue %d   ", last[i] = get_qlevel(pids[i]));
    printf(1, "\n");
  }

  for(i = 0; i < NCHILD; i++)
    kill(pids[i]);
  for(i = 0; i < NCHILD; i++)
    wait();
  for(i = 0; i < NCHILD; i++)
    if(last[i] != (kind[i][0] == 'C' ? 2 : 0))
      ok = 0;
  if(ok)
    printf(1, "\nresult: CPU-bound children ended in queue 2, I/O-bound children in queue 0\n");
  else
    printf(1, "\nresult: unexpected final queue levels\n");
  exit();
}
