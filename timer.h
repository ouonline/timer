#ifndef __TIMER_H__
#define __TIMER_H__

int timer_init(void);
int timer_add(int delay, int interval, void* arg,
              void (*func)(void*), void (*dtor)(void*));
void timer_del(void* arg, void (*func)(void*));
void timer_destroy(void);

#endif
