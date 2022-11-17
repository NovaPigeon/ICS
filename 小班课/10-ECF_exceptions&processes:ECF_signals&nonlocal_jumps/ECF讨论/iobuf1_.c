#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
int main(void)
{
    int i;
    for (i = 0; i < 2; i++)
    {
        fork();
        printf("Hello!\n");
    }

    wait(NULL);
    wait(NULL);

    return 0;
}