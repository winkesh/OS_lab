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
  int burst[NCHILD] = {40, 10, 30, 20};
  int pids[NCHILD], done[2], go[2];
  int i, j, pid, t0;

  printf(1, "sjftest: 4 CPU-bound children declare bursts A=%d B=%d C=%d D=%d ticks\n",
         burst[0], burst[1], burst[2], burst[3]);
  printf(1, "with set_burst() right after fork, then wait at a start barrier so all\n");
  printf(1, "four are ready together. SJF must finish them in order B, D, C, A.\n\n");
  pipe(done);
  pipe(go);

  for(i = 0; i < NCHILD; i++){
    pid = fork();
    if(pid < 0){
      printf(1, "fork failed\n");
      exit();
    }
    if(pid == 0){
      int start, end, me = getpid();
      close(done[0]);
      close(go[1]);
      if(set_burst(me, burst[i]) < 0)
        printf(1, "set_burst failed\n");
      read(go[0], &t0, sizeof(t0));
      start = uptime();
      spin(burst[i]);
      end = uptime();
      printf(1, "  child %c pid %d burst %d finished: ready %d, first ran %d, done %d, turnaround %d\n",
             'A' + i, me, burst[i], t0, start, end, end - t0);
      write(done[1], &me, sizeof(me));
      exit();
    }
    pids[i] = pid;
  }
  if(set_burst(getpid(), -5) == -1)
    printf(1, "parent: set_burst rejects a negative burst as expected\n");

  sleep(5);
  t0 = uptime();
  printf(1, "parent: all children have declared their bursts, releasing them at tick %d\n", t0);
  for(i = 0; i < NCHILD; i++)
    write(go[1], &t0, sizeof(t0));
  close(go[1]);
  close(done[1]);

  for(i = 0; i < NCHILD; i++)
    wait();

  printf(1, "\ncompletion order:");
  for(i = 0; i < NCHILD; i++){
    read(done[0], &pid, sizeof(pid));
    for(j = 0; j < NCHILD; j++)
      if(pids[j] == pid)
        printf(1, " %d. %c (burst %d)", i + 1, 'A' + j, burst[j]);
  }
  printf(1, "\n");
  exit();
}
