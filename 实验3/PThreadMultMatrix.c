#define ACCELERATE_NEW_LAPACK
#define ACCELERATE_LAPACK_ILP64

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <Accelerate/Accelerate.h>  // 使用 Accelerate 框架

// 全局矩阵指针
double *A, *B, *C;
// 矩阵维度
int M, N, K;  
// 线程数
int num_threads;

// 线程数据结构
typedef struct {
    int thread_id;
    int start_row;
    int end_row;
} thread_data_t;

/**
 * 线程函数：计算 C 的 [start_row, end_row) 行区间
 */
void *thread_work(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    for (int i = data->start_row; i < data->end_row; i++) {
        for (int j = 0; j < K; j++) {
            double sum = 0.0;
            for (int k = 0; k < N; k++) {
                sum += A[i * N + k] * B[k * K + j];
            }
            C[i * K + j] = sum;
        }
    }
    pthread_exit(NULL);
}

/**
 * 获取当前时间 (秒)
 */
double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1.0e-9;
}

/**
 * 比较 C 与 C_blas 两个矩阵的最大差异
 */
double compare_results(const double *C, const double *C_blas, int M, int K) {
    double max_diff = 0.0;
    for (int i = 0; i < M * K; i++) {
        double diff = C_blas[i] - C[i];
        if (diff < 0.0) diff = -diff;
        if (diff > max_diff) {
            max_diff = diff;
        }
    }
    return max_diff;
}

/**
 * 使用 Accelerate 的 cblas_dgemm 计算 A*B，并与并行结果 C 比较
 */
void verify_with_blas(const double *A, const double *B, const double *C, int M, int N, int K) {
    // 分配 C_blas
    double *C_blas = (double *)malloc(sizeof(double) * M * K);
    if (!C_blas) {
        fprintf(stderr, "Error: Failed to allocate C_blas\n");
        return;
    }
    // 初始化 C_blas (可选)
    for (int i = 0; i < M * K; i++) {
        C_blas[i] = 0.0;
    }

    // 使用新版 cblas_dgemm 进行矩阵乘法计算
    //  A: M x N, B: N x K, 结果 C_blas: M x K
    cblas_dgemm(
        CblasRowMajor,      // 数据按行存储
        CblasNoTrans,       // A 不转置
        CblasNoTrans,       // B 不转置
        (long long)M,       // A 的行数
        (long long)K,       // B 的列数（结果 C 的列数）
        (long long)N,       // 公共维度
        1.0,                // alpha
        A,                  // 矩阵 A
        (long long)N,       // lda = A 的列数
        B,                  // 矩阵 B
        (long long)K,       // ldb = B 的列数
        0.0,                // beta
        C_blas,             // 输出矩阵 C_blas
        (long long)K        // ldc = C_blas 的列数
    );

    // 计算并打印最大差异
    double max_diff = compare_results(C, C_blas, M, K);
    printf("  >> Max difference (Pthreads vs BLAS) = %e\n", max_diff);

    free(C_blas);
}

int main() {
    // 要测试的线程数
    int thread_options[] = {1, 2, 4, 8, 16};
    int num_thread_options = sizeof(thread_options) / sizeof(thread_options[0]);

    // 要测试的矩阵规模（方阵）
    int size_options[] = {128, 256, 512, 1024, 2048};
    int num_size_options = sizeof(size_options) / sizeof(size_options[0]);

    // 打印表头
    printf("MatrixSize, Threads, Time(s)\n");

    // 遍历矩阵规模
    for (int s = 0; s < num_size_options; s++) {
        int dim = size_options[s];
        M = dim;  // 行数
        N = dim;  // 公共维度
        K = dim;  // 列数

        // 遍历线程数
        for (int t = 0; t < num_thread_options; t++) {
            num_threads = thread_options[t];

            // 分配矩阵内存
            A = (double *)malloc(sizeof(double) * M * N);
            B = (double *)malloc(sizeof(double) * N * K);
            C = (double *)malloc(sizeof(double) * M * K);
            if (!A || !B || !C) {
                fprintf(stderr, "Error: Memory allocation failed\n");
                return -1;
            }

            // 随机初始化 A, B
            // 若要结果可重复，可用固定种子，如 srand(12345)
            srand((unsigned int)time(NULL));
            for (int i = 0; i < M * N; i++) {
                A[i] = (double)(rand() % 100) / 10.0;
            }
            for (int i = 0; i < N * K; i++) {
                B[i] = (double)(rand() % 100) / 10.0;
            }

            // 创建线程数组
            pthread_t *threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
            thread_data_t *thread_data = (thread_data_t *)malloc(num_threads * sizeof(thread_data_t));
            if (!threads || !thread_data) {
                fprintf(stderr, "Error: Thread memory allocation failed\n");
                return -1;
            }

            // 启动计时
            double start_time = get_time();

            // 按行划分任务给每个线程
            int rows_per_thread = M / num_threads;
            int remainder = M % num_threads;
            int current_row = 0;
            for (int i = 0; i < num_threads; i++) {
                thread_data[i].thread_id = i;
                thread_data[i].start_row = current_row;
                int assigned_rows = rows_per_thread + ((i < remainder) ? 1 : 0);
                thread_data[i].end_row = current_row + assigned_rows;
                current_row += assigned_rows;

                pthread_create(&threads[i], NULL, thread_work, &thread_data[i]);
            }

            // 等待所有线程结束
            for (int i = 0; i < num_threads; i++) {
                pthread_join(threads[i], NULL);
            }

            // 结束计时
            double end_time = get_time();
            double elapsed = end_time - start_time;

            // 输出当前测试结果
            printf("%d, %d, %f\n", dim, num_threads, elapsed);

            // 使用 Accelerate BLAS 验证结果正确性
            verify_with_blas(A, B, C, M, N, K);

            // 释放资源
            free(threads);
            free(thread_data);
            free(A);
            free(B);
            free(C);
        }
    }
    return 0;
}