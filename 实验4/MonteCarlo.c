#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>

typedef struct {
    long long points;       // 本线程要生成的随机点数
    unsigned int seed;      // rand_r 的种子
    long long in_circle;    // 本线程统计的落在圆内的点数
} thread_arg_t;

// 线程函数：生成随机点并统计落在单位圆内的个数
void* thread_func(void *arg) {
    thread_arg_t *t = (thread_arg_t*)arg;
    long long in_cnt = 0;
    for (long long i = 0; i < t->points; ++i) {
        double x = (double)rand_r(&t->seed) / RAND_MAX;
        double y = (double)rand_r(&t->seed) / RAND_MAX;
        if (x*x + y*y <= 1.0) {
            ++in_cnt;
        }
    }
    t->in_circle = in_cnt;
    return NULL;
}

int main(void) {
    // 待测试的投点总数
    long long n_values[]      = { 1000, 5000, 10000, 20000, 50000 };
    int       num_n           = sizeof(n_values) / sizeof(n_values[0]);
    // 待测试的线程数
    const int thread_options[] = { 1, 2, 4, 8, 16 };
    int       num_options     = sizeof(thread_options) / sizeof(thread_options[0]);

    // 打印表头
    printf("     n\tThreads\t       m\t    pi_est\t time(s)\n");
    printf("---------------------------------------------------------------\n");

    // 遍历所有 n 与线程数组合
    for (int ni = 0; ni < num_n; ++ni) {
        long long n = n_values[ni];
        for (int t = 0; t < num_options; ++t) {
            int num_threads = thread_options[t];

            // 动态分配线程句柄与参数数组
            pthread_t    *threads = malloc(sizeof(pthread_t) * num_threads);
            thread_arg_t *args    = calloc(num_threads, sizeof(thread_arg_t));

            // 将 n 均分到各线程，最后一个线程分配余数
            long long base = n / num_threads;
            long long rem  = n % num_threads;
            for (int i = 0; i < num_threads; ++i) {
                args[i].points    = base + (i == num_threads - 1 ? rem : 0);
                args[i].seed      = (unsigned int)time(NULL) ^ (i * 0x9e3779b1);
                args[i].in_circle = 0;
            }

            // 记录开始时间
            struct timeval t_start, t_end;
            gettimeofday(&t_start, NULL);

            // 创建并启动线程
            for (int i = 0; i < num_threads; ++i) {
                pthread_create(&threads[i], NULL, thread_func, &args[i]);
            }
            // 等待线程结束并汇总各线程的统计结果
            long long total_in = 0;
            for (int i = 0; i < num_threads; ++i) {
                pthread_join(threads[i], NULL);
                total_in += args[i].in_circle;
            }

            // 记录结束时间
            gettimeofday(&t_end, NULL);
            double elapsed = (t_end.tv_sec - t_start.tv_sec)
                           + (t_end.tv_usec - t_start.tv_usec) / 1e6;

            // 估算 π
            double pi_est = 4.0 * (double)total_in / (double)n;

            // 输出一行结果
            printf("%8lld\t%7d\t%7lld\t%10.8f\t%8.6f\n",
                   n, num_threads, total_in, pi_est, elapsed);

            free(threads);
            free(args);
        }
    }

    return EXIT_SUCCESS;
}