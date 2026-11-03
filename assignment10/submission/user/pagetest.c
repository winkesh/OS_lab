// A10 Q3: inspect a page, make it read-only, and show the write fault.
#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
  char *p;
  int pid;

  p = sbrk(4096);
  if(p == (char*)-1){
    printf(2, "pagetest: sbrk failed\n");
    exit();
  }
  p[0] = 'A';                                   // writable: this works
  printf(1, "allocated one page at 0x%x, wrote '%c'\n", p, p[0]);

  printf(1, "\n--- before setreadonly ---\n");
  pageinfo((int)p);

  if(setreadonly((int)p) < 0){
    printf(2, "pagetest: setreadonly failed\n");
    exit();
  }
  printf(1, "\n--- after setreadonly(0x%x) ---\n", p);
  pageinfo((int)p);
  printf(1, "reading still works: p[0] = '%c'\n", p[0]);

  printf(1, "\n--- error cases ---\n");
  printf(1, "pageinfo(0x40000000)      = %d\n", pageinfo(0x40000000));
  printf(1, "setreadonly(0x40000000)   = %d (unmapped)\n", setreadonly(0x40000000));
  printf(1, "setreadonly(0x80100000)   = %d (kernel page, refused)\n", setreadonly(0x80100000));

  printf(1, "\n--- writing to the read-only page (in a child) ---\n");
  pid = fork();                                 // copyuvm keeps the PTE flags, so the child's copy is read-only too
  if(pid == 0){
    printf(1, "child %d: writing 'B' to 0x%x ...\n", getpid(), p);
    p[0] = 'B';                                 // page fault: trap 14, err 7 (P|W|U)
    printf(1, "child: ERROR, write to read-only page succeeded\n");
    exit();
  }
  wait();
  printf(1, "parent: child %d was killed by the page fault, p[0] still '%c'\n", pid, p[0]);
  exit();
}
