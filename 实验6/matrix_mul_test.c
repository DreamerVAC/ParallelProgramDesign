#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "parallel_for.h"

// functor 参数结构
typedef struct {
    int M, N, P;
    float *A, *B, *C;
} MatMulArgs;

// functor：对单个元素 (row, col) 进行乘加
void matmul_functor(int idx, void *arg) {
    MatMulArgs *a = (MatMulArgs*)arg;
    int row = idx / a->P;
    int col = idx % a->P;
    float sum = 0;
    for (int k = 0; k < a->N; ++k) {
        sum += a->A[row * a->N + k] * a->B[k * a->P + col];
    }
    a->C[row * a->P + col] = sum;
}

int main() {
    int sizes[] = {128, 256, 512, 1024, 2048};
    int thread_counts[] = {1, 2, 4, 8, 16};

    for (int si = 0; si < sizeof(sizes)/sizeof(sizes[0]); ++si) {
        int size = sizes[si];
        for (int ti = 0; ti < sizeof(thread_counts)/sizeof(thread_counts[0]); ++ti) {
            int threads = thread_counts[ti];

            float *A = malloc(sizeof(float) * size * size);
            float *B = malloc(sizeof(float) * size * size);
            float *C = malloc(sizeof(float) * size * size);

            for (int i = 0; i < size * size; ++i) A[i] = (float)(rand()) / RAND_MAX;
            for (int i = 0; i < size * size; ++i) B[i] = (float)(rand()) / RAND_MAX;

            MatMulArgs args = {size, size, size, A, B, C};
            int total = size * size;

            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);

            parallel_for(0, total, 1, matmul_functor, &args, threads);

            clock_gettime(CLOCK_MONOTONIC, &t1);
            double elapsed = (t1.tv_sec - t0.tv_sec)
                           + (t1.tv_nsec - t0.tv_nsec) * 1e-9;

            printf("Size=%d, Threads=%d, Time=%.6f s\n", size, threads, elapsed);

            free(A);
            free(B);
            free(C);
        }
    }
    return 0;
}