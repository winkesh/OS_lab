# OS Lab Assignment 10: Physical Memory, Address Translation, Page Protection and Page Replacement

Built on a clean copy of xv6-public (x86). The Makefile drops `-Werror` and uses `-smp N,sockets=N`. New system calls: `memstats` (22), `vtop` (23), `pageinfo` (24) and `setreadonly` (25), wired through `syscall.h/.c`, `sysproc.c`, `defs.h`, `user.h` and `usys.S`. Every kernel change is tagged `// A10 Qn`. Run with `make qemu-nox CPUS=1` and then `memtest`, `vttest`, `pagetest` and `pagereplace [frames]`. Logs are in `output_logs/` and screenshots in `screenshots/` (appended below). Stock `usertests` still prints **ALL TESTS PASSED** (2 CPUs).

## Q1: Tracking physical memory (`memstats`)

**Changes.** `kmem` in `kalloc.c` gets four counters, updated under `kmem.lock`: `nalloc` (successful `kalloc()` calls), `nfreed` (`kfree()` calls), `freepages` (length of the free list) and `totalpages`. During boot, `kinit1`/`kinit2` seed the free list by calling `kfree()` on every page. Those calls are not real frees, so a `ready` flag is set at the end of `kinit2()`. Only after that does `kfree()` count toward `nfreed`. `totalpages` is the free-list size at that moment. `kmemstats()` copies all four values under the lock so they are consistent with each other. `sys_memstats()` prints total/allocated/freed/currently allocated (`total - free`, which is the same as `nalloc - nfreed`) and available pages, and returns the currently allocated count.

**Output.** 57000 pages (228000 KB, from the end of the kernel to `PHYSTOP`) are managed. Before the test, 211 pages are in use (kernel stacks, page tables and the memory of init, sh and memtest). After `sbrk(16*4096)`, total allocated rises by exactly **16** (490 → 506) and free pages drop by 16. After `sbrk(-16*4096)`, total freed rises by 16 (214 → 230) and the in-use count goes back to 211. Stock `sbrk` is eager (`growproc` → `allocuvm`), and the heap stays below 4 MB, so no extra page-table page appears.

## Q2: Virtual-to-physical translation (`vtop`)

**Changes.** `walkpgdir()` is `static`, so `vtop(pgdir, va)` lives in `vm.c`. It calls `walkpgdir(pgdir, va, 0)`; `alloc = 0` means a lookup never creates page tables. It returns -1 if no PTE exists or `PTE_P` is clear. Otherwise it returns `PTE_ADDR(*pte) | (va & 0xFFF)`: the frame address plus the page offset. `sys_vtop()` reads the argument with `argint` and uses the current process's `pgdir`.

**Output.** `vttest` grows the heap by 3 pages (0x3000–0x6000). Code (`0x0`), stack and heap addresses all translate. `0x3000` and `0x3123` land in the same frame with the offset kept (`0xDEE3000` / `0xDEE3123`). The three heap pages sit in non-contiguous frames. Three cases come back **INVALID**: `0x6000` (just above `sz`: the page table exists but the PTE is not present), `0x40000000` (no page table at all), and heap page 2 after `sbrk(-4096)` freed it. A kernel address (`0x80100000` → `0x100000`) does translate, because every process's page directory maps the kernel half; its `PTE_U` is clear, so user code still cannot touch it.

## Q3: Page protection (`pageinfo`, `setreadonly`)

**Changes.** `pageinfo(pgdir, va)` in `vm.c` uses `walkpgdir` the same way. If the page is mapped, it prints the raw PTE, frame number (`PTE_ADDR >> 12`), frame address and the P/W/U bits, and returns 0. Otherwise it prints "not mapped" and returns -1. `setreadonly(pgdir, va)` refuses `va >= KERNBASE` and any PTE that is not present or not `PTE_U`. Without this check, a user process could write-protect kernel mappings that are shared by every page directory. It then clears `PTE_W` and reloads `cr3`, so a stale writable TLB entry cannot keep the page writable.

**Output.** The new page at `0x3000` starts as PTE `0xdf2b067` (P=1 W=1 U=1). After `setreadonly` it is `0xdf2b065` (W=0); the frame (57131) is the same and reads still work. Unmapped and kernel addresses are rejected with -1. `fork` copies PTE flags (`copyuvm`), so the child's copy is read-only too. When the child writes to it, the CPU raises **trap 14 (page fault), err 7**. Error code 7 = present (1) + write (2) + user mode (4), i.e. a protection violation and not a missing page. `trap.c` kills the child (`pid 6 pagetest: trap 14 err 7 ... addr 0x3000--kill proc`). The parent goes on and its page still holds `'A'`.

## Q4: FIFO, LRU and Optimal (`pagereplace`)

**Program.** `pagereplace [frames [refs...]]` defaults to 3 frames and the given string. Like A9's `pagerepl`, it also accepts a custom string. `simulate()` fills empty frames first. When the frames are full, `victim()` picks the frame to replace. **FIFO** uses a rotating pointer (the oldest load). **LRU** picks the smallest last-use stamp (a hit refreshes the stamp). **OPT** scans forward and picks the page whose next use is farthest away; a page that is never used again counts as infinitely far. Each reference prints the frames and hit/FAULT. The run ends with a table for 3, 4 and 5 frames, computed in integer math because xv6's `printf` has no `%f`.

| Frames | Algorithm | Faults | Hits | Hit ratio | Fault ratio |
|:-:|:-:|:-:|:-:|:-:|:-:|
| 3 | FIFO | **15** | 5 | 25.00% | 75.00% |
| 3 | LRU | **12** | 8 | 40.00% | 60.00% |
| 3 | OPT | **9** | 11 | 55.00% | 45.00% |
| 4 | FIFO | 10 | 10 | 50.00% | 50.00% |
| 4 | LRU | 8 | 12 | 60.00% | 40.00% |
| 4 | OPT | 8 | 12 | 60.00% | 40.00% |
| 5 | FIFO | 9 | 11 | 55.00% | 45.00% |
| 5 | LRU | 7 | 13 | 65.00% | 35.00% |
| 5 | OPT | 7 | 13 | 65.00% | 35.00% |

**Observations.** **OPT performs best** at every frame count. It is optimal by definition, but it needs the future reference string, so a real OS cannot implement it; it serves as the lower bound. LRU is the best practical algorithm here. It uses the recent past to predict the future, and for this string (with heavy reuse of pages 0, 2 and 3) it matches OPT at 4 and 5 frames. With 3 frames it is 3 faults worse. FIFO is worst. It evicts page 0 just because it was loaded early, even though page 0 is the most frequently used page, so the next reference to 0 faults again. 6 of the faults in every row are compulsory (6 distinct pages: 0, 1, 2, 3, 4, 7), so 7 faults at 5 frames is close to the floor. All three algorithms improve as frames are added. LRU and OPT are stack algorithms and can never show Belady's anomaly; FIFO can (shown in A9 with `1,2,3,4,1,2,5,1,2,3,4,5`).
