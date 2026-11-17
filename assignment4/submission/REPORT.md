# OS Lab Assignment 4: SRTF, HRRN and MLFQ Schedulers in xv6

Built on a clean copy of xv6-public (x86). The policy is chosen at build time:

```
make clean
make qemu-nox SCHED=SRTF CPUS=1      # or SCHED=HRRN, SCHED=MLFQ, SCHED=RR (default)
```

New system calls: `set_burst` (22) and `get_qlevel` (23, added so `mlfqtest` can show queue levels). They are wired through `syscall.h/.c`, `sysproc.c`, `defs.h`, `user.h` and `usys.S`. Every kernel change is commented and tagged `// A4 Qn`. The full diff is in `diffs/A4_all_changes.diff`, logs are in `output_logs/` and screenshots in `screenshots/` (appended below). Tests use `CPUS=1`. Stock `usertests` prints **ALL TESTS PASSED** under RR and MLFQ with 2 CPUs. SRTF and HRRN can't run it, because its `preempt` test needs the timer to take the CPU back from a spinning process; `forktest`, `ls` and `echo` work under both.

**Common changes.** `struct proc` gets `ctime`, `burst_time`, `remaining_time`, `queue_level` and `ticks_used`, all initialised in `allocproc()` (`ctime = ticks`, the rest 0). `scheduler()` calls `pickproc()`, which returns the one RUNNABLE process the policy wants. In `trap()`, the timer `yield()` is replaced per policy. **Undeclared bursts:** the assignment doesn't say how to treat a process that never calls `set_burst`, such as `init`, `sh` or a test's parent. Here such a process counts as the *shortest* job (SRTF key 0, HRRN burst 1). These processes only run briefly before blocking, so this keeps the shell responsive and lets a test's parent fork staggered arrivals.

## Q1: SRTF (preemptive)

**Changes.** `set_burst(pid, bt)` sets `burst_time = remaining_time = bt` (returns -1 for a negative `bt` or an unknown pid). `pickproc()` returns the RUNNABLE process with the smallest remaining time; the earlier arrival wins a tie. On every timer tick, `trap()` calls `srtf_tick()`, which decrements the running process's `remaining_time` and returns 1 if any RUNNABLE process has a **strictly** smaller one; only then does `trap()` call `yield()`.

**Test (`srtftest`).** The parent forks A (burst 60), sleeps 15 ticks, forks B (burst 25), sleeps 8 ticks and forks C (burst 6). It calls `set_burst(pid, ...)` on each child right after `fork()`. Each child reports when it has done 25/50/75/100% of its work.

**Output.** A runs alone from tick 12. B arrives at tick 27 and runs straight away; A makes no progress until B is done (A's 25% mark comes only at tick 60). C arrives at tick 35 and cuts into B (B's 25% mark at 33, 50% only at 46). C finishes at 40, then B resumes and finishes at 59, and only then does A resume (finishing at 104). Completion order **C, B, A**: each newly arrived shorter job preempts the longer running one. Under RR (`baseline_RR_srtftest_hrrntest.txt`) all three share the CPU and C, B and A all finish later.

## Q2: HRRN (non-preemptive)

**Changes.** `pickproc()` computes `((ticks - ctime) + burst) * 100 / burst` in integer arithmetic for every RUNNABLE process and picks the highest ratio; the earlier arrival wins a tie. There is no timer preemption. When two or more processes with declared bursts compete, a `[hrrn]` line logs every candidate's ratio.

**Test (`hrrntest`).** S1 and S2 (burst 10) and L (burst 60) arrive together. After that, one more short job arrives every 10 ticks (S3..S10).

**Output.** S1 and S2 run first: with equal waits, a short job's ratio rises 6× faster (at tick 20: S2 200 vs L 116). By tick 30, L has waited 20 ticks, and its ratio beats S3, which has just arrived (ratio 100), so **L runs third, while S3 is waiting**. Shortest-first would have run S3 and every later short job before L, so L would have had to wait for the whole stream. Short jobs still get good turnaround (10–20 ticks, except S3, which waited behind L).

## Q3: MLFQ (3 queues)

**Changes.** New processes start at `queue_level` 0. `pickproc()` finds the highest non-empty queue and takes the next RUNNABLE process in it after the one picked from that queue last time. Each queue has **its own round-robin cursor**: with one shared cursor, the I/O jobs in queue 0 kept moving the cursor so that the same queue-2 process always won, and the other queue-2 processes starved. On every timer tick, `trap()` calls `mlfq_tick()`, which counts `ticks_used`. When that reaches the slice (1, 2 or 4 ticks for queues 0, 1 and 2), the process is demoted one level (queue 2 stays at 2), the counter resets, an `[mlfq]` line is printed, and the process yields. `sleep()` resets `ticks_used`, so a process that blocks before its slice runs out keeps its level.

**Test (`mlfqtest`).** Two CPU-bound children (infinite loops) and two I/O-bound children (a tiny compute, then `sleep(1)`, forever). The parent samples every child's level with `get_qlevel()` every 10 ticks for 10 samples, then kills them all.

**Output.** Each CPU-bound child goes 0 → 1 after 1 tick and 1 → 2 after 2 more, and stays in queue 2. The I/O-bound children are never demoted and sit in queue 0 in every sample. The test prints **"CPU-bound children ended in queue 2, I/O-bound children in queue 0"**. The parent itself is demoted too: printing is CPU work that doesn't block, which is how MLFQ is meant to behave.

## Comparative write-up: turnaround, response and starvation for mixed workloads

`schedmix` (ticks, one CPU). **cpu**: CPU jobs of 60, 30 and 10 ticks, released together. **mix**: the same three plus an I/O job (12 × [¼ tick of compute + `sleep(3)`], ≥ 36 ticks even with no competition). **stream**: a 60-tick job plus 8 short 10-tick jobs, one arriving every ~10 ticks.

| Scheduler | cpu avg TAT | cpu avg resp | mix avg TAT | mix avg resp | mix I/O job TAT | stream: long job finished |
|---|:-:|:-:|:-:|:-:|:-:|:-:|
| RR (baseline) | 64.3 | 1.0 | 59.5 | 1.5 | 43 | 9th of 9 |
| SRTF | **48.0** | 15.6 | **47.0** | 12.7 | **37** | 9th of 9 |
| HRRN | 54.0 | 16.0 | 67.7 | 12.2 | 125 | 1st of 9 |
| MLFQ | 65.0 | 1.0 | 65.7 | 1.5 | 54 | 9th of 9 |

**SRTF** gives the best turnaround in every workload. It always runs the job closest to completion, and because it preempts, the I/O job (remaining time ≈ 1) gets the CPU the moment it wakes, finishing in 37 ticks, close to its 36-tick minimum. The cost is **starvation**: in the stream workload the long job always has the largest remaining time, so it ran only after every short job was done. It also depends on honest burst estimates, and response time for long jobs is poor (15.6 average).

**HRRN** sits between SJF and FCFS. In the cpu workload it still runs the short jobs first (54.0). In hrrntest it **prevents starvation**: the waiting long job's ratio rises until it beats new short arrivals. In the stream it even went first, because every job's ratio starts at 1.0 and L had arrived earliest. Being non-preemptive, it is **bad for mixed workloads**: every time the I/O job woke, it waited for a whole CPU burst to finish, so its turnaround was 125 and average turnaround the worst (67.7).

**MLFQ** needs no burst estimates and gives the best response time (1.0–1.5, like RR), because every new job starts in queue 0. CPU hogs sink to queue 2 and I/O jobs stay high. Its turnaround is close to RR's, not SJF's: jobs are only told apart once they have run, and long jobs in queue 2 still share the CPU. The I/O job did a bit worse than under RR (54 vs 43). Queue level only decides who runs *next*; a higher-queue job that wakes up still waits for the current 4-tick queue-2 slice to end. A process that happens to be running when a tick fires is also charged a whole tick. **Starvation** is possible in principle: a steady stream of high-queue work would leave queue 2 waiting forever, since this simplified version has no periodic priority boost. Here the stream's short jobs also sank to queue 2, so the long job kept getting slices but finished last.

**Trade-off.** SRTF minimises turnaround and handles mixed workloads well, but starves long jobs and needs burst estimates. HRRN fixes the starvation with aging built into the ratio, but as a non-preemptive policy it gives I/O-bound jobs poor turnaround. MLFQ gives the best response time and needs no estimates, so it suits interactive mixed workloads, at the price of RR-like turnaround and possible starvation without a priority boost.
