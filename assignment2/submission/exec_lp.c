#include <stdio.h>
#include <unistd.h>

int main(void)
{
    execlp("ls", "ls", "-l", (char *)NULL);
    perror("execlp");
    return 1;
}
