#include <stdio.h>
#include <time.h>
#include <unistd.h>

int main(void)
{
    int counter = 0;

    for(;;)
    {
        sleep(2);
        printf("Tick: %d\n", counter++);
    }

    return 0;
}