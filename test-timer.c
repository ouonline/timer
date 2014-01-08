#include <stdio.h>
#include "timer.h"

int test1(void* nil)
{
    printf("every 1 s.\n");
    return 0;
}

int test3(void* nil)
{
    printf("every 3 s.\n");
    return 0;
}

int main(void)
{
    timer_init();
    timer_add(0, 1, test1, NULL, NULL);
    timer_add(0, 3, test3, NULL, NULL);

    while (1);

    return 0;
}
