#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mkl.h"

#ifndef N
#define N 256
#endif

static double A[N][N], B[N][N], Cmat[N][N];

// ��ʼ������
void init_matrix() {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = rand() / (double)RAND_MAX;
            B[i][j] = rand() / (double)RAND_MAX;
            Cmat[i][j] = 0.0;
        }
    }
}

// C����ʵ��
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

// ����ѭ��˳��
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

// �����Ż�
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

// ѭ��չ��
double version5_unroll() {
    init_matrix();
    clock_t start = clock();

    for (int i = 0; i < N; i++) {
        for (int k = 0; k < N; k++) {
            // ���� N �� 4 �ı���
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
    // MKL �� cblas_dgemm �ӿڰ�һά���鴦��׼��һά����
    static double Amat[N * N], Bmat[N * N], Cmat2[N * N];

    for (int i = 0; i < N * N; i++) {
        Amat[i] = rand() / (double)RAND_MAX;
        Bmat[i] = rand() / (double)RAND_MAX;
        Cmat2[i] = 0.0;
    }

    double alpha = 1.0, beta = 0.0;
    // row-major ģʽ��, A �� m*k, B �� k*n, C �� m*n
    clock_t start = clock();

    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
        N, N, N, alpha, Amat, N, Bmat, N, beta, Cmat2, N);

    clock_t end = clock();
    return (double)(end - start) / CLOCKS_PER_SEC;
}

int main() {
    srand((unsigned)time(NULL));

    double t2 = version2_basic();
    printf("[�汾2] C ����ʵ��: %f ��\n", t2);

    double t3 = version3_reorder();
    printf("[�汾3] ����ѭ��˳��: %f ��\n", t3);

    double t4 = version4_optimized();
    printf("[�汾4] �����Ż�(ͬv3�߼�): %f ��\n", t4);

    double t5 = version5_unroll();
    printf("[�汾5] ѭ��չ��: %f ��\n", t5);

    double t6 = version6_mkl();
    printf("[�汾6] Intel MKL: %f ��\n", t6);

    return 0;
}
