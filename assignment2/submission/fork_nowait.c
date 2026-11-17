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
    printf("I am parent, PID = %d\n", getpid());
    waitpid(pid, NULL, 0);
    return 0;
}
