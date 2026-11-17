#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define TICK_ITERS 5000000
#define MAXJOB 12

struct job {
  char *name;
  int cpu;
  int io_rounds;
  int burst;
};

struct result {
  int id;
  int arrive;
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

static int out;

static void
col(char *s, int w)
{
  int n = strlen(s);
  printf(out, "%s", s);
  while(n++ < w)
    printf(out, " ");
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

static void
child(struct job *j, int id, int go, int done, int barrier)
{
  struct result r;

  set_burst(getpid(), j->burst);
  r.id = id;
  r.arrive = uptime();
  if(barrier)
    read(go, &r.arrive, sizeof(r.arrive));
  r.start = uptime();
  run(j);
  r.end = uptime();
  write(done, &r, sizeof(r));
  exit();
}

int
main(int argc, char *argv[])
{
  struct job jobs[MAXJOB] = {
    {"long-cpu", 60, 0, 60},
    {"mid-cpu", 30, 0, 30},
    {"short-cpu", 10, 0, 10},
    {"io-bound", 0, 12, 1},
  };
  struct result res[MAXJOB];
  static char report[2048];
  int njob, i, pid, t0, done[2], go[2], stream;
  int sum_tat = 0, sum_resp = 0, longpos = 0, after = 0;

  if(argc != 2 || (strcmp(argv[1], "cpu") && strcmp(argv[1], "mix") && strcmp(argv[1], "stream"))){
    printf(2, "usage: schedmix cpu|mix|stream\n");
    exit();
  }
  stream = strcmp(argv[1], "stream") == 0;
  njob = strcmp(argv[1], "cpu") == 0 ? 3 : 4;
  if(stream){
    njob = 9;
    for(i = 1; i < njob; i++){
      jobs[i].name = "short-cpu";
      jobs[i].cpu = 10;
      jobs[i].io_rounds = 0;
      jobs[i].burst = 10;
    }
  }
  pipe(done);
  pipe(go);

  for(i = 0; i < njob; i++){
    if(stream && i >= 2)
      sleep(10);
    pid = fork();
    if(pid < 0){
      printf(1, "fork failed\n");
      exit();
    }
    if(pid == 0){
      close(done[0]);
      close(go[1]);
      child(&jobs[i], i, go[0], done[1], !stream);
    }
  }
  if(!stream){
    sleep(5);
    t0 = uptime();
    for(i = 0; i < njob; i++)
      write(go[1], &t0, sizeof(t0));
  }
  close(go[1]);
  close(done[1]);
  for(i = 0; i < njob; i++)
    wait();
  for(i = 0; i < njob; i++)
    read(done[0], &res[i], sizeof(res[i]));

  unlink("schedmix.out");
  out = open("schedmix.out", O_CREATE | O_RDWR);
  printf(out, "workload '%s'\n", argv[1]);
  printf(out, "order job        burst arrived response turnaround\n");
  for(i = 0; i < njob; i++){
    struct job *j = &jobs[res[i].id];
    col("", 2);
    ncol(i + 1, 4);
    col(j->name, 11);
    ncol(j->burst, 6);
    ncol(res[i].arrive, 8);
    ncol(res[i].start - res[i].arrive, 9);
    ncol(res[i].end - res[i].arrive, 0);
    printf(out, "\n");
    sum_resp += res[i].start - res[i].arrive;
    sum_tat += res[i].end - res[i].arrive;
    if(res[i].id == 0)
      longpos = i;
  }
  printf(out, "average response %d.%d ticks, average turnaround %d.%d ticks\n",
         sum_resp / njob, (sum_resp * 10 / njob) % 10, sum_tat / njob, (sum_tat * 10 / njob) % 10);
  if(stream){
    for(i = longpos + 1; i < njob; i++)
      after++;
    printf(out, "long-cpu finished %d of %d, %d short jobs finished after it\n", longpos + 1, njob, after);
  }
  close(out);
  out = open("schedmix.out", O_RDONLY);
  i = read(out, report, sizeof(report));
  close(out);
  unlink("schedmix.out");
  write(1, report, i);
  exit();
}
