// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file
                   // defined by the kernel linker script in kernel.ld

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
  // A10 Q1: page accounting, protected by kmem.lock once use_lock is set
  int ready;        // 0 while kinit1/kinit2 seed the free list, 1 afterwards
  uint totalpages;  // pages handed to the allocator at boot
  uint nalloc;      // total successful kalloc() calls since boot
  uint nfreed;      // total kfree() calls since boot (boot-time seeding not counted)
  uint freepages;   // pages currently on the free list
} kmem;

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;
  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
  kmem.totalpages = kmem.freepages;  // A10 Q1: everything seeded so far is the pool
  kmem.ready = 1;                    // A10 Q1: from now on kfree() counts as a free
}

void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((uint)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
    kfree(p);
}
//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(v, 1, PGSIZE);

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = (struct run*)v;
  r->next = kmem.freelist;
  kmem.freelist = r;
  kmem.freepages++;      // A10 Q1: one more page on the free list
  if(kmem.ready)
    kmem.nfreed++;       // A10 Q1: count real frees, not the boot-time freerange()
  if(kmem.use_lock)
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r;

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    kmem.freepages--;    // A10 Q1: page leaves the free list
    kmem.nalloc++;       // A10 Q1: count successful allocations
  }
  if(kmem.use_lock)
    release(&kmem.lock);
  return (char*)r;
}


// A10 Q1: snapshot the counters under the lock so the four values agree.
void
kmemstats(uint *total, uint *nalloc, uint *nfreed, uint *freepages)
{
  acquire(&kmem.lock);
  *total = kmem.totalpages;
  *nalloc = kmem.nalloc;
  *nfreed = kmem.nfreed;
  *freepages = kmem.freepages;
  release(&kmem.lock);
}
