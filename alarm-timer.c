#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>

#include "../memorypool/mm.h"
#include "../kernel-utils/list.h"

struct alarm_timer {
    int interval, remain;
    struct list_node node;
    void* arg;
    int (*func)(void*);
    void (*dtor)(void*);
};

/*---------------------------------------------------------------------------*/

static struct list_node timer_list;
static pthread_mutex_t h_lock;

static inline void alarm_timer_start(void)
{
    struct itimerval itv;

    itv.it_interval.tv_sec = 1;
    itv.it_interval.tv_usec = 0;
    itv.it_value.tv_sec = 1;
    itv.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, NULL);
}

static inline void alarm_timer_stop(void)
{
    struct itimerval itv;

    itv.it_interval.tv_sec = 0;
    itv.it_interval.tv_usec = 0;
    itv.it_value.tv_sec = 0;
    itv.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, NULL);
}

static inline void do_alarm_timer_del(struct alarm_timer* t)
{
    __list_del(&t->node);

    if (t->dtor)
        t->dtor(t->arg);

    mm_free(t);
}

static inline void alarm_timer_action(void)
{
    struct list_node *p, *n;

    pthread_mutex_lock(&h_lock);
    list_for_each_safe (p, n, &timer_list) {
        struct alarm_timer* t = list_entry(p, struct alarm_timer, node);

        if (t->remain > 0)
            --t->remain;
        else {
            register int err = t->func(t->arg);
            if (err)
                do_alarm_timer_del(t);
            else
                t->remain = t->interval - 1;
        }
    }
    pthread_mutex_unlock(&h_lock);
}

static void sigalrm_action(int sig, siginfo_t* s, void* nil)
{
    alarm_timer_action();
}

static int install_alarm_handler(void)
{
    struct sigaction handler;

    sigemptyset(&handler.sa_mask);
    sigaddset(&handler.sa_mask, SIGALRM);
    handler.sa_flags = SA_SIGINFO;
    handler.sa_sigaction = sigalrm_action;

    if (sigaction(SIGALRM, &handler, NULL) != 0) {
        perror("sigaction");     
        return -1;
    }

    return 0;
}

static inline int alarm_timer_init(void)
{
    int err;

    list_init(&timer_list);
    pthread_mutex_init(&h_lock, NULL);

    err = install_alarm_handler();
    if (!err)
        alarm_timer_start();

    return err;
}

static inline void alarm_timer_exit(void)
{
    struct list_node *p, *n;

    alarm_timer_stop();
    pthread_mutex_destroy(&h_lock);

    list_for_each_safe (p, n, &timer_list)
        do_alarm_timer_del(list_entry(p, struct alarm_timer, node));
}

static inline int alarm_timer_add(int delay, int interval, int (*f)(void*),
                                  void (*d)(void*), void* arg)
{
    struct alarm_timer* t;

    if (interval <= 0)
        return -1;

    if (delay < 0)
        delay = 0;

    t = mm_alloc(sizeof(struct alarm_timer));
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

static inline void __alarm_timer_del(int (*f)(void*), void* arg)
{
    struct list_node *p, *n;

    list_for_each_safe (p, n, &timer_list) {
        struct alarm_timer* t = list_entry(p, struct alarm_timer, node);

        if (t->func == f && t->arg == arg) {
            do_alarm_timer_del(t);
            return;
        }
    }
}

static inline void alarm_timer_del(int (*f)(void*), void* arg)
{
    pthread_mutex_lock(&h_lock);
    __alarm_timer_del(f, arg);
    pthread_mutex_unlock(&h_lock);
}

/*---------------------------------------------------------------------------*/

int timer_init(void)
{
    return alarm_timer_init();
}

void timer_exit(void)
{
    alarm_timer_exit();
}

int timer_add(int delay, int interval, int (*f)(void*),
              void (*d)(void*), void* arg)
{
    return alarm_timer_add(delay, interval, f, d, arg);
}

void timer_del(int (*f)(void*), void* arg)
{
    alarm_timer_del(f, arg);
}
