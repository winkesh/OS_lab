#include "types.h"
#include "stat.h"
#include "user.h"

#define NCHILD 3
#define TICK_ITERS 5000000

static void
spin(int t)
{
  volatile int x = 0;
  int i, j;
  for(i = 0; i < t; i++)
    for(j = 0; j < TICK_ITERS; j++)
      x++;
}

int
main(void)
{
  int work[NCHILD] = {60, 40, 20};
  int pids[NCHILD], done[2];
  int i, j, pid, t0;

  printf(1, "fcfstest: 3 CPU-bound children created one after another.\n");
  printf(1, "A is created first with the most work (~60 ticks), C last with the least (~20).\n");
  printf(1, "FCFS must finish them in creation order A, B, C.\n\n");
  pipe(done);

  for(i = 0; i < NCHILD; i++){
    spin(3);
    t0 = uptime();
    pid = fork();
    if(pid < 0){
      printf(1, "fork failed\n");
      exit();
    }
    if(pid == 0){
      int start, end, me = getpid();
      close(done[0]);
      start = uptime();
      spin(work[i]);
      end = uptime();
      printf(1, "  child %c pid %d finished: created %d, first ran %d, done %d, turnaround %d\n",
             'A' + i, me, t0, start, end, end - t0);
      write(done[1], &me, sizeof(me));
      exit();
    }
    pids[i] = pid;
    printf(1, "parent: created child %c pid %d at tick %d (work ~%d ticks)\n", 'A' + i, pid, t0, work[i]);
  }
  close(done[1]);

  for(i = 0; i < NCHILD; i++)
    wait();

  printf(1, "\ncompletion order:");
  for(i = 0; i < NCHILD; i++){
    read(done[0], &pid, sizeof(pid));
    for(j = 0; j < NCHILD; j++)
      if(pids[j] == pid)
        printf(1, " %d. %c (pid %d)", i + 1, 'A' + j, pid);
  }
  printf(1, "\n");
  exit();
}
