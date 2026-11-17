# OS Lab

Operating Systems lab assignments 1–10. Each assignment folder holds the assignment PDF and the submission.

## Installation and setup

### 1. Toolchain (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y git build-essential gcc-multilib qemu-system-x86
```

`gcc-multilib` is needed because xv6 is built as 32-bit x86 (`-m32`). `qemu-system-x86` provides `qemu-system-i386`, which `make qemu-nox` runs. A1 only needs `bash` and `bc`; A2 and A6 only need `gcc` and `make`.

### 2. Google Antigravity CLI (`agy`)

```bash
curl -fsSL https://antigravity.google/cli/install.sh | bash
export PATH="$HOME/.local/bin:$PATH"     # add this line to ~/.bashrc if agy is "command not found"
agy --version
```

There is no separate login command: the first prompt that needs the model opens a browser to sign in with your Google account. Over SSH it prints a URL instead; paste the code back within ~30 seconds.

```bash
agy -p "say hi"
```

If you see `Illegal instruction (core dumped)` inside a VM, set the VM's CPU type to `host` and cold-boot it.

### 3. Get the repo and point agy at it

```bash
git clone git@github.com:winkesh/OS_lab.git
git clone https://github.com/mit-pdos/xv6-public.git xv6-clean
cd OS_lab
agy --add-dir . -p "Read README.md and assignment4/submission/REPORT.md, then explain how to build and run assignment 4"
```

Without `--add-dir`, agy works in its own scratch folder instead of the repo. Don't use `--dangerously-skip-permissions` outside a throwaway directory or VM.

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

The `REPORT.md` (A5: `README.md`) in each submission has the exact run commands. A1 and A6 have no report; their scripts and programs run as described above.
