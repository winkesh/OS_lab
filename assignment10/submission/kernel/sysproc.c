#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// A10 Q1: print the kernel page-allocator counters.
// Returns the number of pages currently allocated.
int
sys_memstats(void)
{
  uint total, nalloc, nfreed, freepages;

  kmemstats(&total, &nalloc, &nfreed, &freepages);
  cprintf("  total pages managed : %d (%d KB)\n", total, total * 4);
  cprintf("  total allocated     : %d\n", nalloc);
  cprintf("  total freed         : %d\n", nfreed);
  cprintf("  currently allocated : %d\n", total - freepages);
  cprintf("  available (free)    : %d (%d KB)\n", freepages, freepages * 4);
  return total - freepages;
}

// A10 Q2: vtop(va) -> physical address of va in the calling process, or -1.
int
sys_vtop(void)
{
  int va;

  if(argint(0, &va) < 0)
    return -1;
  return vtop(myproc()->pgdir, (uint)va);
}

// A10 Q3: pageinfo(va) prints frame number and P/W/U flags; -1 if unmapped.
int
sys_pageinfo(void)
{
  int va;

  if(argint(0, &va) < 0)
    return -1;
  return pageinfo(myproc()->pgdir, (uint)va);
}

// A10 Q3: setreadonly(va) clears PTE_W on the user page holding va.
int
sys_setreadonly(void)
{
  int va;

  if(argint(0, &va) < 0)
    return -1;
  return setreadonly(myproc()->pgdir, (uint)va);
}
