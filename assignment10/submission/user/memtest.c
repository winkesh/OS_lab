// A10 Q1: watch the kernel page-allocator counters around sbrk().
#include "types.h"
#include "stat.h"
#include "user.h"

#define NPAGES 16

int
main(void)
{
  int before, grown, after, i;
  char *p;

  printf(1, "=== memstats: before sbrk ===\n");
  before = memstats();

  p = sbrk(NPAGES * 4096);              // stock sbrk allocates the frames immediately
  if(p == (char*)-1){
    printf(2, "memtest: sbrk failed\n");
    exit();
  }
  for(i = 0; i < NPAGES; i++)           // use every page
    p[i * 4096] = i;
  printf(1, "\n=== memstats: after sbrk(%d * 4096) at 0x%x ===\n", NPAGES, p);
  grown = memstats();
  printf(1, "  -> currently allocated grew by %d pages (expected %d)\n", grown - before, NPAGES);

  sbrk(-NPAGES * 4096);                 // give the pages back (deallocuvm -> kfree)
  printf(1, "\n=== memstats: after sbrk(-%d * 4096) ===\n", NPAGES);
  after = memstats();
  printf(1, "  -> currently allocated dropped by %d pages, back to start: %s\n",
         grown - after, after == before ? "yes" : "no");
  exit();
}
