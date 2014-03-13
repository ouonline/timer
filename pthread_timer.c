#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "../mm/mm.h"
#include "../kernel-utils/list.h"
#include "../threadpool/c/threadpool.h"

struct pthread_timer {
    struct list_node node;
    int interval, remain;
    void* arg;
    void (*func)(void*);
    void (*dtor)(void*);
};

/*---------------------------------------------------------------------------*/

static struct list_node g_timer_list;
static pthread_mutex_t g_list_lock;
static pthread_t g_timer_thread;
static struct thread_pool* g_thread_pool;

static inline void do_pthread_timer_del(struct pthread_timer* t)
{
    __list_del(&t->node);

    if (t->dtor)
        t->dtor(t->arg);

    mm_free(t);
}

static void* pthread_timer_func(void* nil)
{
    while (1) {
        struct list_node* p;

        pthread_mutex_lock(&g_list_lock);
        list_for_each (p, &g_timer_list) {
            struct pthread_timer* t = list_entry(p, struct pthread_timer, node);
            if (t->remain > 0)
                --t->remain;
            else {
                thread_pool_add_task(g_thread_pool, t->arg, t->func);
                t->remain = t->interval - 1;
            }
        }
        pthread_mutex_unlock(&g_list_lock);

        sleep(1);
    }

    return NULL;
}

static inline int pthread_timer_init(void)
{
    int err = -1;

    list_init(&g_timer_list);
    pthread_mutex_init(&g_list_lock, NULL);

    g_thread_pool = thread_pool_init(0);
    if (g_thread_pool) {
        err = pthread_create(&g_timer_thread, NULL, pthread_timer_func, NULL);
        if (err) {
            thread_pool_destroy(g_thread_pool);
            g_thread_pool = NULL;
        }
    }

    return err;
}

static inline void pthread_timer_destroy(void)
{
    struct list_node *p, *n;

    pthread_cancel(g_timer_thread);
    pthread_join(g_timer_thread, NULL);
    pthread_mutex_destroy(&g_list_lock);

    list_for_each_safe (p, n, &g_timer_list)
        do_pthread_timer_del(list_entry(p, struct pthread_timer, node));

    if (g_thread_pool)
        thread_pool_destroy(g_thread_pool);
}

static inline int pthread_timer_add(int delay, int interval, void* arg,
                                    void (*f)(void*), void (*d)(void*))
{
    struct pthread_timer* t;

    if (interval <= 0)
        return -1;

    if (delay < 0)
        delay = 0;

    t = mm_alloc(sizeof(struct pthread_timer));
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

static inline void __pthread_timer_del(void* arg, void (*f)(void*))
{
    struct list_node *p, *n;

    list_for_each_safe (p, n, &g_timer_list) {
        struct pthread_timer* t = list_entry(p, struct pthread_timer, node);

        if (t->func == f && t->arg == arg) {
            do_pthread_timer_del(t);
            return;
        }
    }
}

static inline void pthread_timer_del(void* arg, void (*f)(void*))
{
    pthread_mutex_lock(&g_list_lock);
    __pthread_timer_del(arg, f);
    pthread_mutex_unlock(&g_list_lock);
}

/*---------------------------------------------------------------------------*/

int timer_init(void)
{
    return pthread_timer_init();
}

void timer_destroy(void)
{
    pthread_timer_destroy();
}

int timer_add(int delay, int interval, void* arg,
              void (*f)(void*), void (*d)(void*))
{
    return pthread_timer_add(delay, interval, arg, f, d);
}

void timer_del(void* arg, void (*f)(void*))
{
    pthread_timer_del(arg, f);
}
