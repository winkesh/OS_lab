CC = gcc
CFLAGS = -Wall -Wextra
PROGS = fork_nowait fork_wait exec_l exec_lp exec_vp runcmd

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGS)
