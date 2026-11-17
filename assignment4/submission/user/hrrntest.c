#include "types.h"
#include "stat.h"
#include "user.h"

#define NSHORT 10
#define NJOB (NSHORT + 1)
#define LONG 60
#define SHORT 10
#define TICK_ITERS 5000000

struct result {
  int id;
  int arrive;
  int start;
  int end;
};

static void
spin(int t)
{
  volatile int x = 0;
  int i, j;
  for(i = 0; i < t; i++)
    for(j = 0; j < TICK_ITERS; j++)
      x++;
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

static int
mkjob(int id, int bt, int done)
{
  struct result r;
  int pid;

  r.id = id;
  r.arrive = uptime();
  pid = fork();
  if(pid < 0){
    printf(1, "fork failed\n");
    exit();
  }
  if(pid == 0){
    r.start = uptime();
    spin(bt);
    r.end = uptime();
    write(done, &r, sizeof(r));
    exit();
  }
  set_burst(pid, bt);
  return pid;
}

int
main(void)
{
  struct result res[NJOB];
  int pids[NJOB], done[2];
  int i, k, lpos = 0, before = 0, after = 0;

  printf(1, "hrrntest: S1, S2 (burst %d) and L (burst %d) arrive together, then one more\n", SHORT, LONG);
  printf(1, "short job arrives every %d ticks (S3..S%d). Shortest-first would leave L\n", SHORT, NSHORT);
  printf(1, "until the stream is over; HRRN must run L once its wait has grown.\n\n");
  pipe(done);

  pids[1] = mkjob(1, SHORT, done[1]);
  pids[2] = mkjob(2, SHORT, done[1]);
  pids[0] = mkjob(0, LONG, done[1]);
  for(i = 3; i <= NSHORT; i++){
    sleep(SHORT);
    pids[i] = mkjob(i, SHORT, done[1]);
  }
  close(done[1]);
  for(i = 0; i < NJOB; i++)
    wait();
  for(i = 0; i < NJOB; i++)
    read(done[0], &res[i], sizeof(res[i]));

  printf(1, "\norder job  pid burst arrived started done wait turnaround\n");
  for(i = 0; i < NJOB; i++){
    k = res[i].id;
    if(k == 0)
      lpos = i;
    col("", 2);
    ncol(i + 1, 4);
    if(k == 0)
      col("L", 5);
    else {
      printf(1, "S");
      ncol(k, 4);
    }
    ncol(pids[k], 4);
    ncol(k == 0 ? LONG : SHORT, 6);
    ncol(res[i].arrive, 8);
    ncol(res[i].start, 8);
    ncol(res[i].end, 5);
    ncol(res[i].start - res[i].arrive, 5);
    ncol(res[i].end - res[i].arrive, 0);
    printf(1, "\n");
  }
  for(i = 0; i < NJOB; i++){
    if(res[i].id == 0)
      continue;
    if(i < lpos && res[i].arrive > res[lpos].arrive)
      before++;
    if(i > lpos && res[i].arrive <= res[lpos].start)
      after++;
  }
  printf(1, "\nL arrived at tick %d and started at tick %d (after waiting %d ticks), in\n",
         res[lpos].arrive, res[lpos].start, res[lpos].start - res[lpos].arrive);
  printf(1, "position %d of %d. %d short jobs that arrived after L still ran before it\n",
         lpos + 1, NJOB, before);
  printf(1, "(short jobs first), but %d short jobs that were already waiting when L\n", after);
  printf(1, "started ran after it: L's ratio had grown past theirs. Shortest-first would\n");
  printf(1, "have run every waiting short job first, so L would wait for the whole stream.\n");
  exit();
}
