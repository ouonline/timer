#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>

#include "../mm/mm.h"
#include "../kernel-utils/list.h"
#include "../threadpool/threadpool.h"

struct alarm_timer {
    struct list_node node;
    int interval, remain;
    void* arg;
    void (*func)(void*);
    void (*dtor)(void*);
};

/*---------------------------------------------------------------------------*/

static struct list_node g_timer_list;
static pthread_mutex_t g_list_lock;
static struct thread_pool* g_thread_pool;

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

    pthread_mutex_lock(&g_list_lock);
    list_for_each_safe (p, n, &g_timer_list) {
        struct alarm_timer* t = list_entry(p, struct alarm_timer, node);

        if (t->remain > 0)
            --t->remain;
        else {
            thread_pool_add_task(g_thread_pool, t->arg, t->func);
            t->remain = t->interval - 1;
        }
    }
    pthread_mutex_unlock(&g_list_lock);
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

    list_init(&g_timer_list);
    pthread_mutex_init(&g_list_lock, NULL);

    err = install_alarm_handler();
    if (!err) {
        g_thread_pool = thread_pool_init(0);
        if (g_thread_pool)
            alarm_timer_start();
        else
            err = -1;
    }

    return err;
}

static inline void alarm_timer_destroy(void)
{
    struct list_node *p, *n;

    alarm_timer_stop();
    pthread_mutex_destroy(&g_list_lock);

    list_for_each_safe (p, n, &g_timer_list)
        do_alarm_timer_del(list_entry(p, struct alarm_timer, node));

    if (g_thread_pool) {
        thread_pool_destroy(g_thread_pool);
        g_thread_pool = NULL;
    }
}

static inline int alarm_timer_add(int delay, int interval, void* arg,
                                  void (*f)(void*), void (*d)(void*))
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

    pthread_mutex_lock(&g_list_lock);
    list_add_prev(&t->node, &g_timer_list);
    pthread_mutex_unlock(&g_list_lock);

    return 0;
}

static inline void __alarm_timer_del(void* arg, void (*f)(void*))
{
    struct list_node *p, *n;

    list_for_each_safe (p, n, &g_timer_list) {
        struct alarm_timer* t = list_entry(p, struct alarm_timer, node);

        if (t->func == f && t->arg == arg) {
            do_alarm_timer_del(t);
            return;
        }
    }
}

static inline void alarm_timer_del(void* arg, void (*f)(void*))
{
    pthread_mutex_lock(&g_list_lock);
    __alarm_timer_del(arg, f);
    pthread_mutex_unlock(&g_list_lock);
}

/*---------------------------------------------------------------------------*/

int timer_init(void)
{
    return alarm_timer_init();
}

void timer_destroy(void)
{
    alarm_timer_destroy();
}

int timer_add(int delay, int interval, void* arg,
              void (*f)(void*), void (*d)(void*))
{
    return alarm_timer_add(delay, interval, arg, f, d);
}

void timer_del(void* arg, void (*f)(void*))
{
    alarm_timer_del(arg, f);
}
