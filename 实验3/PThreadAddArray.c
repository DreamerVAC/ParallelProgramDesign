#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

typedef struct {
    int start;          // 当前线程处理起始下标
    int end;            // 当前线程处理结束下标
    long long partial_sum;  // 当前线程计算的局部和
} thread_arg_t;

int *array;   // 全局数组指针

// 线程入口函数：计算局部和，不涉及共享变量的写操作
void* parallel_sum_without_mutex(void* arg) {
    thread_arg_t *targ = (thread_arg_t*) arg;
    long long sum = 0;
    for (int i = targ->start; i < targ->end; i++) {
        sum += array[i];
    }
    targ->partial_sum = sum;
    return NULL;
}

int main() {
    int sizes[8] = {1000000, 2000000, 4000000, 8000000, 16000000,32000000,64000000,128000000};
    int thread_nums[5] = {1, 2, 4, 8, 16};
    srand(time(NULL));

    for (int i = 0; i < 8; i++) {
        int array_size = sizes[i];
        // Allocate memory for the array and initialize with random numbers (0-9)
        array = (int*)malloc(sizeof(int) * array_size);
        for (int j = 0; j < array_size; j++) {
            array[j] = rand() % 10;
        }
        printf("----- 数组规模: %d -----\n", array_size);

        // Loop over different thread counts
        for (int k = 0; k < 5; k++) {
            int num_threads = thread_nums[k];
            pthread_t threads[num_threads];
            thread_arg_t thread_args[num_threads];
            int chunk = array_size / num_threads;

            struct timeval start, end;
            gettimeofday(&start, NULL);

            // Create threads; each thread processes its subarray
            for (int t = 0; t < num_threads; t++) {
                thread_args[t].start = t * chunk;
                thread_args[t].end = (t == num_threads - 1) ? array_size : (t + 1) * chunk;
                thread_args[t].partial_sum = 0;
                pthread_create(&threads[t], NULL, parallel_sum_without_mutex, &thread_args[t]);
            }

            // Wait for threads to finish and aggregate results
            long long sum_result = 0;
            for (int t = 0; t < num_threads; t++) {
                pthread_join(threads[t], NULL);
                sum_result += thread_args[t].partial_sum;
            }

            gettimeofday(&end, NULL);
            double time_spent = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec);
            time_spent /= 1000;  // Convert microseconds to milliseconds

            printf("线程数: %d, 求和结果: %lld, 耗时: %.3f ms\n", num_threads, sum_result, time_spent);
        }

        free(array);
    }
    return 0;
}