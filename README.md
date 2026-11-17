# OS Lab

Operating Systems lab assignments 1–10. Each assignment folder holds the assignment PDF and the submission.

## Replicating an assignment

A1, A2 and A6 work straight from the clone, since they're plain scripts and C programs. The xv6 ones (A3, A4, A5, A7–A10) only contain the files that were changed, so you also need a stock xv6 to drop them into:

```bash
git clone git@github.com:winkesh/OS_lab.git
git clone https://github.com/mit-pdos/xv6-public.git xv6
cp OS_lab/assignment4/submission/{kernel,user}/* OS_lab/assignment4/submission/Makefile xv6/
cd xv6 && make clean && make qemu-nox SCHED=SRTF CPUS=1
```

Swap `assignment4` for the one you want. Some differences between them:

- **A3/A4:** pass `SCHED=` to pick the scheduler (A3: `FCFS`, `PRIORITY`, `SJF`; A4: `SRTF`, `HRRN`, `MLFQ`; `RR` is the default for both). Run `make clean` whenever you switch.
- **A7–A10:** plain `make qemu-nox`, no `SCHED`.
- **A5:** the folder is spelled `assigment5`, and its files sit directly in `kernel/`, `user/` and `Makefile`, not under `submission/`. Copy them the same way.
- **A1:** `chmod +x *.sh`, then run the scripts.
- **A2:** `make`, then run the binaries.

Each assignment's `REPORT.md` has the exact run commands.
