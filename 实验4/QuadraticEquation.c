#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <stdbool.h>

// Coefficients and intermediates
double a, b, c;
double b2, four_ac, discriminant, sqrt_disc, root1, root2;

// Synchronization
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv_both = PTHREAD_COND_INITIALIZER;
pthread_cond_t cv_disc = PTHREAD_COND_INITIALIZER;
pthread_cond_t cv_sqrt = PTHREAD_COND_INITIALIZER;

bool ready_b2 = false, ready_4ac = false, ready_disc = false, ready_sqrt = false;

void *thread_compute_b2(void *arg) {
    pthread_mutex_lock(&mtx);
    b2 = b * b;
    ready_b2 = true;
    if (ready_4ac) pthread_cond_broadcast(&cv_both);
    pthread_mutex_unlock(&mtx);
    return NULL;
}

void *thread_compute_4ac(void *arg) {
    pthread_mutex_lock(&mtx);
    four_ac = 4.0 * a * c;
    ready_4ac = true;
    if (ready_b2) pthread_cond_broadcast(&cv_both);
    pthread_mutex_unlock(&mtx);
    return NULL;
}

void *thread_compute_disc(void *arg) {
    pthread_mutex_lock(&mtx);
    while (!(ready_b2 && ready_4ac)) pthread_cond_wait(&cv_both, &mtx);
    discriminant = b2 - four_ac;
    ready_disc = true;
    pthread_cond_broadcast(&cv_disc);
    pthread_mutex_unlock(&mtx);
    return NULL;
}

void *thread_compute_sqrt(void *arg) {
    pthread_mutex_lock(&mtx);
    while (!ready_disc) pthread_cond_wait(&cv_disc, &mtx);
    if (discriminant >= 0) sqrt_disc = sqrt(discriminant);
    ready_sqrt = true;
    pthread_cond_broadcast(&cv_sqrt);
    pthread_mutex_unlock(&mtx);
    return NULL;
}

void *thread_compute_root1(void *arg) {
    pthread_mutex_lock(&mtx);
    while (!ready_sqrt) pthread_cond_wait(&cv_sqrt, &mtx);
    if (discriminant >= 0) root1 = (-b + sqrt_disc) / (2.0 * a);
    pthread_mutex_unlock(&mtx);
    return NULL;
}

void *thread_compute_root2(void *arg) {
    pthread_mutex_lock(&mtx);
    while (!ready_sqrt) pthread_cond_wait(&cv_sqrt, &mtx);
    if (discriminant >= 0) root2 = (-b - sqrt_disc) / (2.0 * a);
    pthread_mutex_unlock(&mtx);
    return NULL;
}

int main() {
    srand((unsigned)time(NULL));
    // Generate random coefficients
    a = (rand() / (double)RAND_MAX) * 200.0 - 100.0;
    b = (rand() / (double)RAND_MAX) * 200.0 - 100.0;
    c = (rand() / (double)RAND_MAX) * 200.0 - 100.0;

    printf("方程: %.3f x^2 + %.3f x + %.3f = 0\n", a, b, c);

    // 单线程计算
    struct timeval ts_start, ts_end;
    gettimeofday(&ts_start, NULL);
    double sb2 = b * b;
    double sfour_ac = 4.0 * a * c;
    double sdisc = sb2 - sfour_ac;
    double ssqrt_disc = (sdisc >= 0) ? sqrt(sdisc) : 0;
    double sroot1 = (sdisc >= 0) ? (-b + ssqrt_disc) / (2.0 * a) : NAN;
    double sroot2 = (sdisc >= 0) ? (-b - ssqrt_disc) / (2.0 * a) : NAN;
    gettimeofday(&ts_end, NULL);
    double serial_time = (ts_end.tv_sec - ts_start.tv_sec)*1e3 + (ts_end.tv_usec - ts_start.tv_usec)*1e-3;

    struct timeval t_start, t_end;
    gettimeofday(&t_start, NULL);

    pthread_t t1, t2, t3, t4, t5, t6;
    pthread_create(&t1, NULL, thread_compute_b2, NULL);
    pthread_create(&t2, NULL, thread_compute_4ac, NULL);
    pthread_create(&t3, NULL, thread_compute_disc, NULL);
    pthread_create(&t4, NULL, thread_compute_sqrt, NULL);
    pthread_create(&t5, NULL, thread_compute_root1, NULL);
    pthread_create(&t6, NULL, thread_compute_root2, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);
    pthread_join(t5, NULL);
    pthread_join(t6, NULL);

    gettimeofday(&t_end, NULL);
    double elapsed = (t_end.tv_sec - t_start.tv_sec)*1e3 + (t_end.tv_usec - t_start.tv_usec)*1e-3;

    if (discriminant < 0) {
        printf("无实数根，判别式 = %.3f\n", discriminant);
    } else {
        printf("根1 = %.6f, 根2 = %.6f\n", root1, root2);
    }
    printf("总耗时：%.3f ms\n", elapsed);

    // 输出单线程耗时
    printf("单线程计算: 根1 = %.6f, 根2 = %.6f, 耗时 %.3f ms\n", sroot1, sroot2, serial_time);
    return 0;
}
