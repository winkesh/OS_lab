#include "types.h"
#include "stat.h"
#include "user.h"

#define TICK_ITERS 5000000
#define MAXJOB 4

struct job {
  char *name;
  int cpu;
  int io_rounds;
  int burst;
  int prio;
};

struct result {
  int id;
  int start;
  int end;
};

static void
spin_iters(int n)
{
  volatile int x = 0;
  int i;
  for(i = 0; i < n; i++)
    x++;
}

static void
spin(int t)
{
  int i;
  for(i = 0; i < t; i++)
    spin_iters(TICK_ITERS);
}

static void
col(char *s, int w)
{
  int n = strlen(s);
  printf(1, "%s", s);
  while(n++ < w)
    printf(1, " ");
}

static void
ncol(int v, int w)
{
  char buf[12];
  int i = sizeof(buf) - 1;
  buf[i] = 0;
  do {
    buf[--i] = '0' + v % 10;
    v /= 10;
  } while(v > 0);
  col(buf + i, w);
}

static void
run(struct job *j)
{
  int r;
  if(j->io_rounds == 0){
    spin(j->cpu);
    return;
  }
  for(r = 0; r < j->io_rounds; r++){
    spin_iters(TICK_ITERS / 4);
    sleep(3);
  }
}

int
main(int argc, char *argv[])
{
  struct job jobs[MAXJOB] = {
    {"long-cpu", 60, 0, 60, 9},
    {"mid-cpu", 30, 0, 30, 6},
    {"short-cpu", 10, 0, 10, 3},
    {"io-bound", 0, 12, 1, 1},
  };
  struct result res[MAXJOB], r;
  int njob, i, pid, t0, done[2], go[2];
  int sum_tat = 0, sum_resp = 0;

  if(argc != 2 || (strcmp(argv[1], "cpu") != 0 && strcmp(argv[1], "mix") != 0)){
    printf(2, "usage: schedmix cpu|mix\n");
    exit();
  }
  njob = strcmp(argv[1], "cpu") == 0 ? 3 : 4;
  pipe(done);
  pipe(go);
  set_priority(getpid(), 1);

  for(i = 0; i < njob; i++){
    pid = fork();
    if(pid < 0){
      printf(1, "fork failed\n");
      exit();
    }
    if(pid == 0){
      close(done[0]);
      close(go[1]);
      set_burst(getpid(), jobs[i].burst);
      read(go[0], &t0, sizeof(t0));
      r.id = i;
      r.start = uptime();
      run(&jobs[i]);
      r.end = uptime();
      write(done[1], &r, sizeof(r));
      exit();
    }
    set_priority(pid, jobs[i].prio);
  }
  sleep(5);
  t0 = uptime();
  for(i = 0; i < njob; i++)
    write(go[1], &t0, sizeof(t0));
  close(go[1]);
  close(done[1]);
  for(i = 0; i < njob; i++)
    wait();
  for(i = 0; i < njob; i++)
    read(done[0], &res[i], sizeof(res[i]));

  printf(1, "workload '%s', all jobs ready at tick %d\n", argv[1], t0);
  printf(1, "order job        burst prio response turnaround\n");
  for(i = 0; i < njob; i++){
    struct job *j = &jobs[res[i].id];
    col("", 2);
    ncol(i + 1, 4);
    col(j->name, 11);
    ncol(j->burst, 6);
    ncol(j->prio, 5);
    ncol(res[i].start - t0, 9);
    ncol(res[i].end - t0, 0);
    printf(1, "\n");
    sum_resp += res[i].start - t0;
    sum_tat += res[i].end - t0;
  }
  printf(1, "average response %d.%d ticks, average turnaround %d.%d ticks\n",
         sum_resp / njob, (sum_resp * 10 / njob) % 10, sum_tat / njob, (sum_tat * 10 / njob) % 10);
  exit();
}
