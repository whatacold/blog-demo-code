/**
 * Implementing queues with condition variable.
 *
 * similar implemention: http://www.cs.nmsu.edu/~jcook/Tools/pthreads/pc.c
 */
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>

typedef struct queue_s queue_t;

struct queue_s {
    int *ringbuf;
    size_t nslots;
    int idx_enq;
    int idx_deq;
    pthread_mutex_t mutex;
    pthread_cond_t cond_not_full;
    pthread_cond_t cond_not_empty;
};

queue_t *
queue_create(size_t n)
{
    queue_t *q;
    int *buf;

    if(0 == n)
        n = 128;

    buf = (int *)malloc(n * sizeof(int));
    if(!buf)
        return NULL;

    q = (queue_t *)malloc(sizeof(*q));
    if(!q) {
        free(buf);
        return NULL;
    }

    q->ringbuf = buf;
    q->nslots = n;
    q->idx_deq = 0;
    q->idx_enq = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond_not_full, NULL);
    pthread_cond_init(&q->cond_not_empty, NULL);

    return q;
}

void
queue_enqueue(queue_t *q, int ele)
{
    assert(q);
    int cnt;

    pthread_mutex_lock(&q->mutex);

    cnt = q->idx_enq + 1 - q->idx_deq;
    while((cnt == 0) || (cnt == q->nslots)) {
        pthread_cond_wait(&q->cond_not_full, &q->mutex);
        /**
         * XXX need recalculation!!!
         */
        cnt = q->idx_enq + 1 - q->idx_deq;
    }
    q->ringbuf[q->idx_enq] = ele;
    q->idx_enq = (q->idx_enq + 1) % (q->nslots);

    pthread_cond_signal(&q->cond_not_empty);

    pthread_mutex_unlock(&q->mutex);
}

void
queue_dequeue(queue_t *q, int *ele)
{
    assert(q);

    pthread_mutex_lock(&q->mutex);

    while(q->idx_enq == q->idx_deq) {
        pthread_cond_wait(&q->cond_not_empty, &q->mutex);
    }
    *ele = q->ringbuf[q->idx_deq];
    q->idx_deq = (q->idx_deq + 1) % (q->nslots);

    pthread_cond_signal(&q->cond_not_full);

    pthread_mutex_unlock(&q->mutex);
}

void
queue_destroy(queue_t *q)
{
    assert(q);

    free(q->ringbuf);
    pthread_cond_destroy(&q->cond_not_full);
    pthread_cond_destroy(&q->cond_not_empty);
    pthread_mutex_destroy(&q->mutex);
}

void *
thr_consumer(void *arg)
{
    queue_t *q = (queue_t *)arg;

    printf("consumer thread created.\n");
    while(1) {
        int ele;

        queue_dequeue(q, &ele);
        printf("consumer: dequeue %d\n", ele);
    }

    printf("consumer thread leaves now.\n");
    pthread_exit(NULL);
    return NULL;
}

void *
thr_producer(void *arg)
{
    queue_t *q = (queue_t *)arg;
    int i;

    printf("producer thread created.\n");
    for(i = 0; i < 255; i++) {
        queue_enqueue(q, i);
        printf("producer: enqueue %d\n", i);
    }

    printf("producer thread leaves now.\n");
    pthread_exit(NULL);
    return NULL;
}

int
main(void)
{
    pthread_t tc, tp;
    int rc;
    queue_t *q;

    printf("main thread pid: %d\n", (int)getpid());

    q = queue_create(128);
    assert(q);

    rc = pthread_create(&tc, NULL, thr_consumer, q);
    assert(!rc);
    rc = pthread_create(&tp, NULL, thr_producer, q);
    assert(!rc);

    pthread_join(tc, NULL);
    pthread_join(tp, NULL);

    queue_destroy(q);

    return 0;
}
