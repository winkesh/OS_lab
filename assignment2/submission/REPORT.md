# OS Lab Assignment 2: fork, exec and wait

Build everything with `make` (or `gcc file.c -o file`). Transcripts are in `output_logs/` and screenshots in `screenshots/`.

## Exercise 1: fork (`fork_nowait.c`, `fork_wait.c`)

`fork_nowait.c` forks; the child prints `I am child` and its PID and exits, the parent prints `I am parent` and its PID and then reaps the child with `waitpid()`. Run on a terminal, the parent printed first in 300 of 300 runs (and in every run in the transcript). After `fork()` the Linux scheduler normally keeps running the parent, so it reaches its `printf` before the child is scheduled. This is not guaranteed: the two processes run concurrently, and on another load or another CPU the child can print first. With stdout redirected to a pipe the order even flips, because the parent's output stays in its buffer until it exits, which is after the child.

**Ensuring the child prints first.** `fork_wait.c` calls `waitpid(pid, &status, 0)` *before* the parent's `printf`. `waitpid` blocks until the child has terminated, so the child's line is always printed first, and the same call reaps the child (no zombie). The parent also reports the child's exit status via `WEXITSTATUS`.

## Exercise 2: exec variants (`exec_l.c`, `exec_lp.c`, `exec_vp.c`)

Each program replaces itself with `ls -l`, so its output is identical to running `ls -l` in the same directory (see `q2_exec.txt`).

| Program | Call | Arguments | PATH search |
|---|---|---|---|
| `exec_l` | `execl("/bin/ls", "ls", "-l", NULL)` | list | no, full path needed |
| `exec_lp` | `execlp("ls", "ls", "-l", NULL)` | list | yes |
| `exec_vp` | `execvp("ls", args)` | `char *args[]` vector | yes |

On success `exec` never returns, so the `perror` after it only runs if the exec fails.

## Exercise 3: `runcmd.c`

`./runcmd <command> <argument>` checks `argc == 3`, otherwise prints `Incorrect number of arguments` and exits 1. It forks; the child calls `execlp(argv[1], argv[1], argv[2], NULL)` (`execlp` because the command name is given without a path). If exec fails, the child prints the error and exits 127. The parent waits with `waitpid`, which reaps the child, and prints `Command successfully completed` only if the child exited normally with status 0. Otherwise it reports the exit status or the signal that killed it.
