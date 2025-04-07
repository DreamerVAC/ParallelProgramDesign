#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

// 打印矩阵（按行打印，每个元素格式化输出）
void print_matrix(double *mat, int rows, int cols) {
    for (int i = 0; i < rows; i++){
        for (int j = 0; j < cols; j++){
            printf("%8.2f ", mat[i * cols + j]);
        }
        printf("\n");
    }
}

// 矩阵乘法：计算 local_C = local_A * B  
// local_A 的尺寸为 local_rows×n, B 尺寸 n×k, 结果 local_C 为 local_rows×k
void matrix_multiply(double *A, double *B, double *C, int local_rows, int n, int k) {
    for (int i = 0; i < local_rows; i++){
        for (int j = 0; j < k; j++){
            C[i * k + j] = 0.0;
            for (int l = 0; l < n; l++){
                C[i * k + j] += A[i * n + l] * B[l * k + j];
            }
        }
    }
}

int main(int argc, char *argv[]){
    int rank, size;
    int m, n, k;       // A: m×n, B: n×k, C: m×k
    int method;        // 0: 块划分, 1: 循环划分, 2: 块循环划分
    int block_size = 16; // 仅对方法2有效，默认块大小

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // 根进程解析命令行参数
    if (rank == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s m n k [method] [block_size]\n", argv[0]);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        m = atoi(argv[1]);
        n = atoi(argv[2]);
        k = atoi(argv[3]);
        method = (argc >= 5) ? atoi(argv[4]) : 0;
        if (method == 2) {
            block_size = (argc >= 6) ? atoi(argv[5]) : 16;
        }
    }
    // 广播 m, n, k, method 以及（对块循环）block_size到所有进程
    MPI_Bcast(&m, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&k, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&method, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if(method == 2)
        MPI_Bcast(&block_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // 所有进程分配 B（全矩阵B每个进程均需保存）
    double *B = (double*) malloc(n * k * sizeof(double));
    // 矩阵 A 和 C 仅在根进程中分配
    double *A = NULL;
    double *C = NULL;
    if (rank == 0) {
        A = (double*) malloc(m * n * sizeof(double));
        C = (double*) malloc(m * k * sizeof(double));
        srand(time(NULL));
        for (int i = 0; i < m * n; i++)
            A[i] = (double)(rand() % 10);
        for (int i = 0; i < n * k; i++)
            B[i] = (double)(rand() % 10);
    }
    // 广播 B 到所有进程
    MPI_Bcast(B, n * k, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // 根据不同划分方式计算每个进程将获得的 A 的行数 local_rows
    int local_rows = 0;
    if (method == 0) { // 块划分
        int rows_per_proc = m / size;
        int remainder = m % size;
        local_rows = (rank < remainder) ? rows_per_proc + 1 : rows_per_proc;
    } else if (method == 1) { // 循环划分
        for (int i = 0; i < m; i++) {
            if (i % size == rank)
                local_rows++;
        }
    } else if (method == 2) { // 块循环划分
        int num_blocks = (m + block_size - 1) / block_size;
        for (int j = 0; j < num_blocks; j++) {
            if (j % size == rank) {
                int start_row = j * block_size;
                int rows_in_block = ((start_row + block_size) <= m) ? block_size : (m - start_row);
                local_rows += rows_in_block;
            }
        }
    }
    // 分配本地 A 和本地结果 C
    double *local_A = (double*) malloc(local_rows * n * sizeof(double));
    double *local_C = (double*) malloc(local_rows * k * sizeof(double));

    /* 数据分发：将全局矩阵 A 按不同方式分发到各进程 */

    if (method == 0) {
        // —— 块划分：利用 MPI_Scatterv 实现连续行分发
        int *sendcounts = (int*) malloc(size * sizeof(int));
        int *displs = (int*) malloc(size * sizeof(int));
        int rows_per_proc = m / size;
        int remainder = m % size;
        for (int i = 0; i < size; i++) {
            sendcounts[i] = (i < remainder ? rows_per_proc + 1 : rows_per_proc) * n;
        }
        displs[0] = 0;
        for (int i = 1; i < size; i++) {
            displs[i] = displs[i - 1] + sendcounts[i - 1];
        }
        MPI_Scatterv(A, sendcounts, displs, MPI_DOUBLE,
                     local_A, sendcounts[rank], MPI_DOUBLE, 0, MPI_COMM_WORLD);
        free(sendcounts);
        free(displs);
    } else if (method == 1) {
        // —— 循环划分：根进程按行下标 i % size 分发
        if (rank == 0) {
            int idx = 0;
            // 根进程复制属于自己的行
            for (int i = 0; i < m; i++) {
                if (i % size == 0) {
                    for (int j = 0; j < n; j++)
                        local_A[idx * n + j] = A[i * n + j];
                    idx++;
                }
            }
            // 对于其他进程，打包并发送其所有行
            for (int p = 1; p < size; p++) {
                int count = 0;
                for (int i = 0; i < m; i++) {
                    if (i % size == p)
                        count++;
                }
                double *temp = (double*) malloc(count * n * sizeof(double));
                int t = 0;
                for (int i = 0; i < m; i++) {
                    if (i % size == p) {
                        for (int j = 0; j < n; j++)
                            temp[t * n + j] = A[i * n + j];
                        t++;
                    }
                }
                MPI_Send(temp, count * n, MPI_DOUBLE, p, 1, MPI_COMM_WORLD);
                free(temp);
            }
        } else {
            MPI_Recv(local_A, local_rows * n, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    } else if (method == 2) {
        // —— 块循环划分：先按块（每块 block_size 行）划分，再循环分配
        int num_blocks = (m + block_size - 1) / block_size;
        if (rank == 0) {
            int idx = 0;
            for (int j = 0; j < num_blocks; j++) {
                if (j % size == 0) {
                    int start_row = j * block_size;
                    int rows_in_block = ((start_row + block_size) <= m) ? block_size : (m - start_row);
                    for (int i = 0; i < rows_in_block; i++) {
                        for (int j2 = 0; j2 < n; j2++) {
                            local_A[idx * n + j2] = A[(start_row + i) * n + j2];
                        }
                        idx++;
                    }
                }
            }
            for (int p = 1; p < size; p++) {
                int count = 0;
                for (int j = 0; j < num_blocks; j++) {
                    if (j % size == p) {
                        int start_row = j * block_size;
                        int rows_in_block = ((start_row + block_size) <= m) ? block_size : (m - start_row);
                        count += rows_in_block;
                    }
                }
                double *temp = (double*) malloc(count * n * sizeof(double));
                int idx2 = 0;
                for (int j = 0; j < num_blocks; j++) {
                    if (j % size == p) {
                        int start_row = j * block_size;
                        int rows_in_block = ((start_row + block_size) <= m) ? block_size : (m - start_row);
                        for (int i = 0; i < rows_in_block; i++) {
                            for (int j2 = 0; j2 < n; j2++) {
                                temp[idx2 * n + j2] = A[(start_row + i) * n + j2];
                            }
                            idx2++;
                        }
                    }
                }
                MPI_Send(temp, count * n, MPI_DOUBLE, p, 1, MPI_COMM_WORLD);
                free(temp);
            }
        } else {
            MPI_Recv(local_A, local_rows * n, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }

    // 同步后计时，开始局部矩阵乘法计算
    MPI_Barrier(MPI_COMM_WORLD);
    double t_start = MPI_Wtime();
    matrix_multiply(local_A, B, local_C, local_rows, n, k);
    double t_end = MPI_Wtime();
    double local_time = t_end - t_start;

    /* 结果收集：将各进程计算得到的局部矩阵 C（尺寸 local_rows×k）汇总成全局矩阵 C */
    if (method == 0) {
        // —— 块划分：利用 MPI_Gatherv 收集
        int *recvcounts = (int*) malloc(size * sizeof(int));
        int *rdispls = (int*) malloc(size * sizeof(int));
        int rows_per_proc = m / size;
        int remainder = m % size;
        for (int i = 0; i < size; i++) {
            recvcounts[i] = (i < remainder ? rows_per_proc + 1 : rows_per_proc) * k;
        }
        rdispls[0] = 0;
        for (int i = 1; i < size; i++) {
            rdispls[i] = rdispls[i - 1] + recvcounts[i - 1];
        }
        MPI_Gatherv(local_C, recvcounts[rank], MPI_DOUBLE,
                    C, recvcounts, rdispls, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        free(recvcounts);
        free(rdispls);
    } else if (method == 1) {
        // —— 循环划分：每个进程发送 local_C，根进程按全局行号组装
        if (rank == 0) {
            // 先将进程0中属于自己的行写入 C
            for (int i = 0, idx = 0; i < m; i++) {
                if (i % size == 0) {
                    for (int j = 0; j < k; j++)
                        C[i * k + j] = local_C[idx * k + j];
                    idx++;
                }
            }
            // 接收其他进程的数据，并按全局行号放置
            for (int p = 1; p < size; p++) {
                int count = 0;
                for (int i = 0; i < m; i++) {
                    if (i % size == p)
                        count++;
                }
                double *temp = (double*) malloc(count * k * sizeof(double));
                MPI_Recv(temp, count * k, MPI_DOUBLE, p, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                for (int i = 0; i < m; i++) {
                    if (i % size == p) {
                        int index = i / size;
                        for (int j = 0; j < k; j++){
                            C[i * k + j] = temp[index * k + j];
                        }
                    }
                }
                free(temp);
            }
        } else {
            MPI_Send(local_C, local_rows * k, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD);
        }
    } else if (method == 2) {
        // —— 块循环划分：根进程按块顺序组装全局 C
        if (rank == 0) {
            int num_blocks = (m + block_size - 1) / block_size;
            int *proc_block_index = (int*) calloc(size, sizeof(int));
            // 处理根进程自身数据
            for (int j = 0; j < num_blocks; j++) {
                if (j % size == 0) {
                    int start_row = j * block_size;
                    int rows_in_block = ((start_row + block_size) <= m) ? block_size : (m - start_row);
                    int idx = proc_block_index[0];
                    for (int i = 0; i < rows_in_block; i++){
                        for (int j2 = 0; j2 < k; j2++){
                            C[(start_row + i) * k + j2] = local_C[(idx + i) * k + j2];
                        }
                    }
                    proc_block_index[0] += rows_in_block;
                }
            }
            // 接收其他进程数据，并按块组装
            for (int p = 1; p < size; p++){
                int count = 0;
                for (int j = 0; j < num_blocks; j++){
                    if(j % size == p) {
                        int start_row = j * block_size;
                        int rows_in_block = ((start_row + block_size) <= m) ? block_size : (m - start_row);
                        count += rows_in_block;
                    }
                }
                double *temp = (double*) malloc(count * k * sizeof(double));
                MPI_Recv(temp, count * k, MPI_DOUBLE, p, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                proc_block_index[p] = 0;
                for (int j = 0; j < num_blocks; j++){
                    if(j % size == p) {
                        int start_row = j * block_size;
                        int rows_in_block = ((start_row + block_size) <= m) ? block_size : (m - start_row);
                        int idx = proc_block_index[p];
                        for (int i = 0; i < rows_in_block; i++){
                            for (int j2 = 0; j2 < k; j2++){
                                C[(start_row + i) * k + j2] = temp[(idx + i) * k + j2];
                            }
                        }
                        proc_block_index[p] += rows_in_block;
                    }
                }
                free(temp);
            }
            free(proc_block_index);
        } else {
            MPI_Send(local_C, local_rows * k, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD);
        }
    }

    double *all_times = NULL;
    if (rank == 0) {
        all_times = (double*) malloc(size * sizeof(double));
        all_times[0] = local_time;
        for (int p = 1; p < size; p++){
            MPI_Recv(&all_times[p], 1, MPI_DOUBLE, p, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        printf("各进程局部计算时间（秒）：\n");
        for (int p = 0; p < size; p++){
            printf("进程 %d: %f\n", p, all_times[p]);
        }
        free(all_times);
    } else {
        MPI_Send(&local_time, 1, MPI_DOUBLE, 0, 3, MPI_COMM_WORLD);
    }

    if (rank == 0) {
        printf("结果矩阵 C (%d x %d):\n", m, k);
        print_matrix(C, m, k);
    }

    free(B);
    free(local_A);
    free(local_C);
    if (rank == 0) {
        free(A);
        free(C);
    }
    MPI_Finalize();
    return 0;
}