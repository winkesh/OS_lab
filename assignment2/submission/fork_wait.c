#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        printf("I am child, PID = %d\n", getpid());
        exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    printf("I am parent, PID = %d (reaped child %d, exit status %d)\n", getpid(), pid, WEXITSTATUS(status));
    return 0;
}
