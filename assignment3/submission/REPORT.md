# OS Lab Assignment 3: FCFS, Priority with Aging, and SJF Schedulers in xv6

Built on a clean copy of xv6-public (x86). The scheduling policy is chosen at build time:

```
make clean
make qemu-nox SCHED=FCFS CPUS=1      # or SCHED=PRIORITY, SCHED=SJF, SCHED=RR (default)
```

`SCHED=X` adds `-DSCHED_X` to `CFLAGS`. Run `make clean` whenever you switch policies so every object is rebuilt. The order tests use `CPUS=1`: with two CPUs, two processes run at the same time, so "strict completion order" no longer makes sense for FCFS/SJF.

New system calls: `set_priority` (22) and `set_burst` (23), wired through `syscall.h/.c`, `sysproc.c`, `defs.h`, `user.h` and `usys.S`. Every kernel change is commented and tagged `// A3 Qn`. `kernel/` holds the modified kernel files, `user/` the test programs, `diffs/A3_all_changes.diff` the full diff against stock xv6, `output_logs/` the transcripts and `screenshots/` the rendered output (appended below). Stock `usertests` still prints **ALL TESTS PASSED** under RR and PRIORITY with 2 CPUs. The `trap 13` line in the log comes from usertests' own `uio` test, which does that on stock xv6 too.

**Common changes.** `struct proc` gets `ctime`, `priority`, `wait_ticks` and `burst_time`, all set in `allocproc()` (`ctime = ticks`, `priority = 10`, the other two 0). The old `scheduler()` loop is split in two. `pickproc()` returns the one RUNNABLE process the policy wants. `scheduler()` switches to that process, resets its `wait_ticks`, and loops again. RR and PRIORITY share a static round-robin cursor: scanning starts one slot after the last process picked, so equal candidates take turns. In `trap()`, the timer-driven `yield()` is compiled out for FCFS and SJF, which makes them non-preemptive: a process keeps the CPU until it exits or blocks.

## Q1: FCFS

**Changes.** `ctime = ticks` in `allocproc()` (`ticks` is read without `tickslock`, because a 32-bit aligned read is atomic and taking `tickslock` under `ptable.lock` would reverse the lock order used by the timer handler). `pickproc()` returns the RUNNABLE process with the smallest `ctime`; pid breaks ties. There is no timer preemption.

**Test (`fcfstest`).** The parent spins ~3 ticks before each `fork()`, so A, B and C get different creation ticks (14, 17, 20). A has the most work (~60 ticks) and C the least (~20). Each child sends its pid through a pipe when it finishes, so the parent prints the true completion order (`wait()` returns zombies in process-table order, not exit order).

**Output.** Completion order **A, B, C**. Each child first runs only when its predecessor finishes (B first ran at tick 80, the tick A finished; C at 120). For contrast, the same program under RR finishes **C, B, A**, because the shortest job finishes first when the CPU is shared (`baseline_RR_fcfstest_sjftest.txt`).

## Q2: Priority-based preemptive scheduling with aging

**Changes.** `priority` defaults to 10. `set_priority(pid, prio)` accepts 1..20, sets the value, resets the wait counter and returns 0 (-1 on a bad priority or pid). `pickproc()` returns the RUNNABLE process with the lowest priority number. Since scanning starts after the last process picked and only a *strictly* better priority replaces the current best, equal priorities take turns (round-robin). The timer `yield()` stays, so a higher-priority process takes over within one tick. **Aging:** after each `ticks++` on CPU 0, `trap()` calls `age()`. That increments `wait_ticks` for every RUNNABLE process; when it exceeds `AGING_TICKS` (30, in `param.h`), the priority number drops by 1 (not below 1), the counter resets, and an `[aging]` line is printed. A process that gets the CPU has its counter reset in `scheduler()`.

**Test (`priotest`).** The parent sets itself to priority 1 so its children cannot preempt it. It then forks A, B, C, D, sets their priorities to 2, 5, 9 and 15, and waits. Every child does the same ~200 ticks of CPU work. The parent also checks that 0 and 21 are rejected.

**Output.** A (priority 2) runs alone from tick 13. B, C and D sit in the queue and age every 31 ticks. B reaches 2 at tick 106 and starts sharing the CPU with A round-robin. C reaches 2 at tick 230. **D, the lowest-priority process, climbs from 15 to 2 and first runs at tick 417, while B and C are still unfinished**, so it does not wait for everyone else to finish and nothing starves. Completion order **A, B, C, D**: higher priority finishes first.

## Q3: SJF with declared burst times

**Changes.** `set_burst(pid, bt)` stores `bt` (ticks, must be ≥ 0; returns -1 otherwise or for an unknown pid). `pickproc()` returns the RUNNABLE process with the smallest `burst_time`. A burst of 0 (never set) is treated as larger than every real burst, so such processes, including `init` and `sh`, run last. Arrival time breaks ties. There is no timer preemption.

**Test (`sjftest`).** Four children each call `set_burst(getpid(), bt)` right after `fork()`, declaring A=40, B=10, C=30, D=20, and then block on a "go" pipe. Without this barrier, the first child scheduled would run to completion right after declaring its burst (non-preemptive), before its siblings had even declared theirs. The parent sleeps 5 ticks so all four can declare, then writes the release tick into the pipe, which makes all four RUNNABLE at the same moment. Each child's CPU work matches its declared burst.

**Output.** Completion order **B, D, C, A** (10, 20, 30, 40). Each job starts the tick the previous one ends: turnarounds are 10, 30, 59 and 99. Under RR, the same jobs finish in the same order, but all of them take longer (37, 69, 89, 101) because they all share the CPU until the end.

## Comparative write-up: turnaround time for CPU-bound vs mixed workloads

`schedmix` releases jobs together (through the same barrier) and measures each job's response time (release to first run) and turnaround (release to finish). Jobs are created in the order long, mid, short, io; bursts and priorities are declared honestly (a shorter job gets a higher priority, and the I/O job gets priority 1). **cpu**: three CPU-bound jobs of 60, 30 and 10 ticks. **mix**: the same three plus an I/O-bound job that does 12 rounds of ~¼ tick of compute followed by `sleep(3)` (≥ 36 ticks even on an idle machine). One CPU, values in ticks:

| Scheduler | cpu: avg turnaround | cpu: avg response | mix: avg turnaround | mix: avg response | mix: I/O job turnaround |
|---|:-:|:-:|:-:|:-:|:-:|
| RR (baseline) | 63.6 | 1.0 | 61.0 | 1.5 | 43 |
| FCFS | 85.6 | 51.6 | 92.5 | 59.7 | 132 |
| Priority + aging | 47.6 | 15.6 | **46.0** | 12.7 | **36** |
| SJF | **45.3** | 15.0 | 64.2 | 11.5 | 119 |

**CPU-bound workload: SJF gives the best turnaround.** When every job needs the CPU continuously and the bursts are known, running the shortest first minimises average turnaround: each job waits only behind shorter ones. SJF provably gives the minimum average waiting time for a batch that arrives together. Here priority scheduling matches it (47.6) only because the priorities were set to follow job length. **FCFS is worst (85.6)** because of the convoy effect: the 60-tick job was created first, so the 10-tick job waits 92 ticks behind it. RR (63.6) sits in between: it avoids the convoy, but because time slices interleave every job, even the short job finishes late (29 vs 9 ticks) and the long one gains nothing.

**Mixed workload: preemptive priority gives the best turnaround.** An I/O-bound job needs the CPU in short bursts and then blocks. Under a non-preemptive scheduler, each time it wakes up it waits behind whatever CPU job is running until that job *finishes*. Under SJF the I/O job had the smallest burst and ran first, but after its first `sleep` it waited behind each CPU job in turn: turnaround **119** (vs a minimum of ~36), and average turnaround jumped from 45.3 to 64.2. FCFS is worse still (132), because the I/O job arrived last. **Priority with preemption** lets the I/O job (priority 1) take the CPU within one tick of waking, and the CPU jobs fill the gaps while it sleeps, so it finishes in **36** ticks (its own sleep time) and the CPU jobs lose almost nothing (10/41/97). Average turnaround is the best, at 46.0. RR also serves the I/O job quickly (43) and has the best response time, but it still pays the CPU-bound interleaving penalty.

**Conclusion.** For pure CPU-bound batches, SJF (or anything that runs short jobs first) gives the lowest turnaround, and FCFS the highest. Once I/O-bound work is mixed in, non-preemption is the problem: SJF and FCFS make the interactive job wait behind whole CPU bursts. A preemptive priority scheduler that favours the I/O job gives the best turnaround and good response. Aging costs a few extra context switches but guarantees that a low-priority job eventually runs (Q2). SJF also depends on honest burst estimates; a job that under-declares its burst can hog the CPU.
