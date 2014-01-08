#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "../kernel-utils/list.h"

struct pthread_timer {
    int interval, remain;
    struct list_node node;
    void* arg;
    int (*func)(void*);
    void (*dtor)(void*);
};

/*---------------------------------------------------------------------------*/

static struct list_node timer_list;
static pthread_t timer_thread;
static pthread_mutex_t h_lock;

static inline void do_pthread_timer_del(struct pthread_timer* t)
{
    __list_del(&t->node);

    if (t->dtor)
        t->dtor(t->arg);

    free(t);
}

static void* pthread_timer_func(void* nil)
{
    while (1) {
        struct list_node *p, *n;

        pthread_mutex_lock(&h_lock);
        list_for_each_safe (p, n, &timer_list) {
            struct pthread_timer* t = list_entry(p, struct pthread_timer, node);

            if (t->remain > 0)
                --t->remain;
            else {
                register int err = t->func(t->arg);
                if (err)
                    do_pthread_timer_del(t);
                else
                    t->remain = t->interval - 1;
            }
        }
        pthread_mutex_unlock(&h_lock);

        sleep(1);
    }

    return NULL;
}

static inline int pthread_timer_init(void)
{
    list_init(&timer_list);
    pthread_mutex_init(&h_lock, NULL);

    return pthread_create(&timer_thread, NULL, pthread_timer_func, NULL);
}

static inline void pthread_timer_exit(void)
{
    struct list_node *p, *n;

    pthread_cancel(timer_thread);
    pthread_join(timer_thread, NULL);
    pthread_mutex_destroy(&h_lock);

    list_for_each_safe (p, n, &timer_list)
        do_pthread_timer_del(list_entry(p, struct pthread_timer, node));
}

static inline int pthread_timer_add(int delay, int interval, int (*f)(void*),
                                    void (*d)(void*), void* arg)
{
    struct pthread_timer* t;

    if (interval <= 0)
        return -1;

    if (delay < 0)
        delay = 0;

    t = malloc(sizeof(struct pthread_timer));
    if (!t)
        return -1;

    t->remain = delay;
    t->interval = interval;
    t->func = f;
    t->dtor = d;
    t->arg = arg;

    pthread_mutex_lock(&h_lock);
    list_add_prev(&t->node, &timer_list);
    pthread_mutex_unlock(&h_lock);

    return 0;
}

static inline void __pthread_timer_del(int (*f)(void*), void* arg)
{
    struct list_node *p, *n;

    list_for_each_safe (p, n, &timer_list) {
        struct pthread_timer* t = list_entry(p, struct pthread_timer, node);

        if (t->func == f && t->arg == arg) {
            do_pthread_timer_del(t);
            return;
        }
    }
}

static inline void pthread_timer_del(int (*f)(void*), void* arg)
{
    pthread_mutex_lock(&h_lock);
    __pthread_timer_del(f, arg);
    pthread_mutex_unlock(&h_lock);
}

/*---------------------------------------------------------------------------*/

int timer_init(void)
{
    return pthread_timer_init();
}

void timer_exit(void)
{
    pthread_timer_exit();
}

int timer_add(int delay, int interval, int (*f)(void*),
              void (*d)(void*), void* arg)
{
    return pthread_timer_add(delay, interval, f, d, arg);
}

void timer_del(int (*f)(void*), void* arg)
{
    pthread_timer_del(f, arg);
}
