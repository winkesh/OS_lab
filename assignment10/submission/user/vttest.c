// A10 Q2: translate virtual addresses with vtop().
#include "types.h"
#include "stat.h"
#include "user.h"

void
show(char *what, uint va)
{
  int pa = vtop(va);

  if(pa == -1)
    printf(1, "%s va 0x%x -> INVALID (no present PTE)\n", what, va);
  else
    printf(1, "%s va 0x%x -> pa 0x%x  (frame 0x%x + offset 0x%x)\n",
           what, va, pa, pa & ~0xFFF, va & 0xFFF);
}

int
main(void)
{
  int local = 0;
  char *p;

  p = sbrk(3 * 4096);
  if(p == (char*)-1){
    printf(2, "vttest: sbrk failed\n");
    exit();
  }
  p[0] = p[4096] = p[2 * 4096] = 1;

  printf(1, "sbrk(3 * 4096) returned 0x%x, sz now 0x%x\n\n", p, sbrk(0));
  printf(1, "--- valid user addresses ---\n");
  show("code  main()     ", (uint)main);
  show("stack &local     ", (uint)&local);
  show("heap  page 0     ", (uint)p);
  show("heap  page 0+0x123", (uint)p + 0x123);   // same frame as above, offset kept
  show("heap  page 1     ", (uint)p + 4096);
  show("heap  page 2+0xabc", (uint)p + 2 * 4096 + 0xabc);

  printf(1, "\n--- invalid / unmapped addresses ---\n");
  show("just above sz    ", (uint)p + 3 * 4096);   // PTE exists in the table, not present
  show("no page table    ", 0x40000000);           // PDE itself is empty
  sbrk(-4096);                                      // free heap page 2
  show("freed heap page 2", (uint)p + 2 * 4096);

  printf(1, "\n--- kernel half (present, but PTE_U clear) ---\n");
  show("KERNBASE+0x100000", 0x80100000);
  exit();
}
