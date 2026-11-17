#include "types.h"
#include "stat.h"
#include "user.h"

#define NCHILD 4
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
  int prio[NCHILD] = {2, 5, 9, 15};
  int work = 200;
  int pids[NCHILD], done[2];
  int i, j, pid, t0;

  printf(1, "priotest: 4 CPU-bound children, same work (~%d ticks each),\n", work);
  printf(1, "priorities A=%d B=%d C=%d D=%d (1 = highest). A RUNNABLE process that waits\n",
         prio[0], prio[1], prio[2], prio[3]);
  printf(1, "more than 30 ticks gets its priority number lowered by 1 ([aging] lines).\n\n");
  pipe(done);

  set_priority(getpid(), 1);
  t0 = uptime();
  for(i = 0; i < NCHILD; i++){
    pid = fork();
    if(pid < 0){
      printf(1, "fork failed\n");
      exit();
    }
    if(pid == 0){
      int start, end, me = getpid();
      close(done[0]);
      start = uptime();
      printf(1, "  child %c pid %d (priority %d) first ran at tick %d\n", 'A' + i, me, prio[i], start);
      spin(work);
      end = uptime();
      printf(1, "  child %c pid %d (priority %d) finished at tick %d, turnaround %d\n",
             'A' + i, me, prio[i], end, end - t0);
      write(done[1], &me, sizeof(me));
      exit();
    }
    pids[i] = pid;
    if(set_priority(pid, prio[i]) < 0)
      printf(1, "set_priority(%d, %d) failed\n", pid, prio[i]);
  }
  printf(1, "parent: created A..D (pids %d %d %d %d) at tick %d\n", pids[0], pids[1], pids[2], pids[3], t0);
  if(set_priority(getpid(), 0) == -1 && set_priority(getpid(), 21) == -1)
    printf(1, "parent: set_priority rejects 0 and 21 as expected\n");
  close(done[1]);

  for(i = 0; i < NCHILD; i++)
    wait();

  printf(1, "\ncompletion order:");
  for(i = 0; i < NCHILD; i++){
    read(done[0], &pid, sizeof(pid));
    for(j = 0; j < NCHILD; j++)
      if(pids[j] == pid)
        printf(1, " %d. %c (prio %d)", i + 1, 'A' + j, prio[j]);
  }
  printf(1, "\n");
  exit();
}
