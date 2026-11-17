#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Incorrect number of arguments\n");
        fprintf(stderr, "Usage: %s <command> <argument>\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        execlp(argv[1], argv[1], argv[2], (char *)NULL);
        perror("execlp");
        exit(127);
    }

    int status;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        printf("Command successfully completed\n");
        return 0;
    }
    if (WIFEXITED(status))
        printf("Command failed with exit status %d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("Command killed by signal %d\n", WTERMSIG(status));
    return 1;
}
