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
  int burst[NCHILD] = {60, 25, 6};
  int gap[NCHILD] = {15, 8, 0};
  int pids[NCHILD], done[2];
  int i, j, q, pid, t0;

  printf(1, "srtftest: A (burst %d) arrives first, B (burst %d) %d ticks later,\n", burst[0], burst[1], gap[0]);
  printf(1, "C (burst %d) %d ticks after B. Each child reports when it has done\n", burst[2], gap[1]);
  printf(1, "25/50/75/100%% of its work. SRTF must let B and then C cut in.\n\n");
  pipe(done);

  for(i = 0; i < NCHILD; i++){
    t0 = uptime();
    pid = fork();
    if(pid < 0){
      printf(1, "fork failed\n");
      exit();
    }
    if(pid == 0){
      int me = getpid(), start;
      close(done[0]);
      start = uptime();
      printf(1, "  %c pid %d: first ran at tick %d (arrived %d)\n", 'A' + i, me, start, t0);
      for(q = 1; q <= 4; q++){
        spin(burst[i] / 4);
        printf(1, "  %c pid %d: %d%% done at tick %d\n", 'A' + i, me, q * 25, uptime());
      }
      printf(1, "  %c pid %d: finished, turnaround %d\n", 'A' + i, me, uptime() - t0);
      write(done[1], &me, sizeof(me));
      exit();
    }
    set_burst(pid, burst[i]);
    pids[i] = pid;
    printf(1, "parent: tick %d: %c (pid %d) arrives with burst %d\n", t0, 'A' + i, pid, burst[i]);
    if(gap[i])
      sleep(gap[i]);
  }
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
