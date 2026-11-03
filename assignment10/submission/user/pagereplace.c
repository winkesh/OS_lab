// A10 Q4: FIFO, LRU and Optimal page replacement.
// usage: pagereplace [frames [ref1 ref2 ... | ref1,ref2,...]]
//   no arguments = 3 frames + the assignment's reference string
#include "types.h"
#include "stat.h"
#include "user.h"

#define MAXF 10
#define MAXR 64
#define FIFO 0
#define LRU  1
#define OPT  2

char *name[] = {"FIFO", "LRU", "OPT"};
int defrefs[] = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1};

// pick the frame to replace for refs[t]; frames are all full here
int
victim(int alg, int *frame, int *stamp, int nf, int *refs, int n, int t, int *next)
{
  int k, j, v = 0, far = -1, dist;

  if(alg == FIFO){                   // oldest load, frames filled in order -> rotating pointer
    v = *next;
    *next = (*next + 1) % nf;
    return v;
  }
  if(alg == LRU){                    // smallest last-use time
    for(k = 1; k < nf; k++)
      if(stamp[k] < stamp[v])
        v = k;
    return v;
  }
  for(k = 0; k < nf; k++){           // OPT: page used farthest in the future (or never)
    for(j = t + 1; j < n && refs[j] != frame[k]; j++)
      ;
    dist = j;                        // j == n means never used again
    if(dist > far){
      far = dist;
      v = k;
    }
  }
  return v;
}

// run one algorithm, return the number of faults; verbose prints every step
int
simulate(int alg, int *refs, int n, int nf, int verbose)
{
  int frame[MAXF], stamp[MAXF];
  int used = 0, next = 0, faults = 0;
  int t, k, hit, v;

  if(verbose)
    printf(1, "\n--- %s, %d frames ---\n", name[alg], nf);

  for(t = 0; t < n; t++){
    hit = -1;
    for(k = 0; k < used; k++)
      if(frame[k] == refs[t])
        hit = k;

    if(hit >= 0){
      stamp[hit] = t;                // only LRU looks at this
    } else {
      faults++;
      if(used < nf)
        v = used++;                  // free frame first
      else
        v = victim(alg, frame, stamp, nf, refs, n, t, &next);
      frame[v] = refs[t];
      stamp[v] = t;
    }

    if(verbose){
      printf(1, "ref %d :", refs[t]);
      for(k = 0; k < nf; k++){
        if(k < used)
          printf(1, "  %d", frame[k]);
        else
          printf(1, "  -");
      }
      printf(1, "   %s\n", hit >= 0 ? "hit" : "FAULT");
    }
  }

  if(verbose)
    printf(1, "%s total page faults: %d, hits: %d\n", name[alg], faults, n - faults);
  return faults;
}

// x/n as a percentage with 2 decimals (xv6 printf has no %f)
void
pct(int x, int n)
{
  int r = x * 10000 / n;
  printf(1, "%s%d.%d%d%%", r < 1000 ? " " : "", r / 100, (r / 10) % 10, r % 10);
}

int
main(int argc, char *argv[])
{
  int refs[MAXR];
  int n, nf, i, f, a, fl;
  char *p;

  nf = argc > 1 ? atoi(argv[1]) : 3;
  if(nf < 1 || nf > MAXF){
    printf(2, "pagereplace: frames must be 1..%d\n", MAXF);
    exit();
  }
  if(argc <= 2){
    n = sizeof(defrefs) / sizeof(defrefs[0]);
    for(i = 0; i < n; i++)
      refs[i] = defrefs[i];
  } else {
    // refs may be separate or comma-separated (xv6 sh allows only 10 args)
    n = 0;
    for(i = 2; i < argc; i++){
      for(p = argv[i]; *p; ){
        if(*p < '0' || *p > '9'){
          p++;
          continue;
        }
        if(n == MAXR){
          printf(2, "pagereplace: at most %d references\n", MAXR);
          exit();
        }
        refs[n] = 0;
        while(*p >= '0' && *p <= '9')
          refs[n] = refs[n] * 10 + (*p++ - '0');
        n++;
      }
    }
    if(n == 0){
      printf(2, "pagereplace: empty reference string\n");
      exit();
    }
  }

  printf(1, "reference string (%d refs):", n);
  for(i = 0; i < n; i++)
    printf(1, " %d", refs[i]);
  printf(1, "\n");

  // (a), (b): step-by-step trace with nf frames
  for(a = FIFO; a <= OPT; a++)
    simulate(a, refs, n, nf, 1);

  // (c): comparison table for 3, 4 and 5 frames
  printf(1, "\nframes | algo | faults | hits | hit ratio | fault ratio\n");
  printf(1, "-------+------+--------+------+-----------+------------\n");
  for(f = 3; f <= 5; f++){
    for(a = FIFO; a <= OPT; a++){
      fl = simulate(a, refs, n, f, 0);
      printf(1, "   %d   | %s%s |   %s%d   |  %s%d  |  ", f, name[a], a == FIFO ? "" : " ",
             fl < 10 ? " " : "", fl, n - fl < 10 ? " " : "", n - fl);
      pct(n - fl, n);
      printf(1, "   |   ");
      pct(fl, n);
      printf(1, "\n");
    }
  }
  exit();
}
