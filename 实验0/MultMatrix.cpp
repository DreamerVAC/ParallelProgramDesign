#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mkl.h"

#ifndef N
#define N 256
#endif

static double A[N][N], B[N][N], Cmat[N][N];

// 初始化函数
void init_matrix() {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = rand() / (double)RAND_MAX;
            B[i][j] = rand() / (double)RAND_MAX;
            Cmat[i][j] = 0.0;
        }
    }
}

// C基础实现
double version2_basic() {
    init_matrix();
    clock_t start = clock();

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                Cmat[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    clock_t end = clock();
    return (double)(end - start) / CLOCKS_PER_SEC;
}

// 调整循环顺序
double version3_reorder() {
    init_matrix();
    clock_t start = clock();

    for (int i = 0; i < N; i++) {
        for (int k = 0; k < N; k++) {
            for (int j = 0; j < N; j++) {
                Cmat[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    clock_t end = clock();
    return (double)(end - start) / CLOCKS_PER_SEC;
}

// 编译优化
double version4_optimized() {
    init_matrix();
    clock_t start = clock();

    for (int i = 0; i < N; i++) {
        for (int k = 0; k < N; k++) {
            for (int j = 0; j < N; j++) {
                Cmat[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    clock_t end = clock();
    return (double)(end - start) / CLOCKS_PER_SEC;
}

// 循环展开
double version5_unroll() {
    init_matrix();
    clock_t start = clock();

    for (int i = 0; i < N; i++) {
        for (int k = 0; k < N; k++) {
            // 假设 N 是 4 的倍数
            for (int j = 0; j < N; j += 4) {
                Cmat[i][j] += A[i][k] * B[k][j];
                Cmat[i][j + 1] += A[i][k] * B[k][j + 1];
                Cmat[i][j + 2] += A[i][k] * B[k][j + 2];
                Cmat[i][j + 3] += A[i][k] * B[k][j + 3];
            }
        }
    }

    clock_t end = clock();
    return (double)(end - start) / CLOCKS_PER_SEC;
}

// Intel MKL
double version6_mkl() {
    // MKL 的 cblas_dgemm 接口按一维数组处理，准备一维数组
    static double Amat[N * N], Bmat[N * N], Cmat2[N * N];

    for (int i = 0; i < N * N; i++) {
        Amat[i] = rand() / (double)RAND_MAX;
        Bmat[i] = rand() / (double)RAND_MAX;
        Cmat2[i] = 0.0;
    }

    double alpha = 1.0, beta = 0.0;
    // row-major 模式下, A 是 m*k, B 是 k*n, C 是 m*n
    clock_t start = clock();

    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
        N, N, N, alpha, Amat, N, Bmat, N, beta, Cmat2, N);

    clock_t end = clock();
    return (double)(end - start) / CLOCKS_PER_SEC;
}

int main() {
    srand((unsigned)time(NULL));

    double t2 = version2_basic();
    printf("[版本2] C 基础实现: %f 秒\n", t2);

    double t3 = version3_reorder();
    printf("[版本3] 调整循环顺序: %f 秒\n", t3);

    double t4 = version4_optimized();
    printf("[版本4] 编译优化(同v3逻辑): %f 秒\n", t4);

    double t5 = version5_unroll();
    printf("[版本5] 循环展开: %f 秒\n", t5);

    double t6 = version6_mkl();
    printf("[版本6] Intel MKL: %f 秒\n", t6);

    return 0;
}
