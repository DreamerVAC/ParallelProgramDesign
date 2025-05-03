#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "parallel_for.h"

#include <pthread.h>

/* per‑thread data */
typedef struct {
    int start;
    int end;
    int inc;
    void (*functor)(int, void *);
    void *arg;
    int tid;
    int num_threads;
} PFTask;

/* worker routine: each thread processes indices in [start,end) with step=inc,
   round‑robin fashion so the workload stays balanced even if (end-start) is
   not divisible by num_threads */
static void *pf_worker(void *p) {
    PFTask *t = (PFTask *)p;
    for (int i = t->start + t->tid * t->inc; i < t->end; i += t->inc * t->num_threads) {
        t->functor(i, t->arg);
    }
    return NULL;
}

void parallel_for(int start, int end, int inc,
                  void (*functor)(int, void *),
                  void *arg, int num_threads)
{
    if (num_threads <= 1) {               /* fallback to serial */
        for (int i = start; i < end; i += inc)
            functor(i, arg);
        return;
    }

    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    PFTask     *tasks  = malloc(num_threads * sizeof(PFTask));

    for (int t = 0; t < num_threads; ++t) {
        tasks[t] = (PFTask){
            .start = start,
            .end   = end,
            .inc   = inc,
            .functor = functor,
            .arg   = arg,
            .tid   = t,
            .num_threads = num_threads
        };
        if (pthread_create(&threads[t], NULL, pf_worker, &tasks[t]) != 0) {
            /* creation failed → fall back to serial for remaining work */
            for (int i = start + t * inc; i < end; i += inc)
                functor(i, arg);
            num_threads = t;              /* only join threads already started */
            break;
        }
    }

    for (int t = 0; t < num_threads; ++t)
        pthread_join(threads[t], NULL);

    free(threads);
    free(tasks);
}