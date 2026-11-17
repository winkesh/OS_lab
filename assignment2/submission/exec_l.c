#include <stdio.h>
#include <unistd.h>

int main(void)
{
    execl("/bin/ls", "ls", "-l", (char *)NULL);
    perror("execl");
    return 1;
}
