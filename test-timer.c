#include <stdio.h>
#include "timer.h"

void test1(void* nil)
{
    printf("every 1 s.\n");
}

void test3(void* nil)
{
    printf("every 3 s.\n");
}

int main(void)
{
    timer_init();
    timer_add(0, 1, NULL, test1, NULL);
    timer_add(0, 3, NULL, test3, NULL);

    while (1);

    return 0;
}
