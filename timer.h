#ifndef __TIMER_H__
#define __TIMER_H__

int timer_init(void);
int timer_add(int delay, int interval,
              int (*func)(void*), void (*dtor)(void*),
              void* arg);
void timer_del(int (*func)(void*), void* arg);
void timer_exit(void);

#endif
