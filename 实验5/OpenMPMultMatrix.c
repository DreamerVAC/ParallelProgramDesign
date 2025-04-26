#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>

static void fill_random(double *mat, int rows, int cols)
{
    for (long long i = 0; i < (long long)rows * cols; ++i)
        mat[i] = (double)rand() / RAND_MAX;          /* [0,1) */
}

static void multiply_omp(const double *A, const double *B, double *C,
                         int m, int n, int k,
                         int num_threads,
                         const char *sched,      /* "default" | "static" | "dynamic" */
                         int chunk)              /* chunk size for static/dynamic */
{
    /* Configure threads and (optionally) schedule policy */
    omp_set_num_threads(num_threads);

    if (strcmp(sched, "default") != 0) {
        omp_sched_t stype = (strcmp(sched, "static")  == 0) ? omp_sched_static  :
                            (strcmp(sched, "dynamic") == 0) ? omp_sched_dynamic :
                                                               omp_sched_guided; /* fallback */
        omp_set_schedule(stype, chunk);
    }

    /* Parallelised i‑loop; inner loops are private per thread */
    #pragma omp parallel for schedule(runtime)
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < k; ++j) {
            double sum = 0.0;
            for (int p = 0; p < n; ++p)
                sum += A[i * (long long)n + p] * B[p * (long long)k + j];
            C[i * (long long)k + j] = sum;
        }
    }
}

static double run_case(int m, int n, int k,
                       int threads, const char *sched)
{
    double *A = (double *)malloc(sizeof(double) * (long long)m * n);
    double *B = (double *)malloc(sizeof(double) * (long long)n * k);
    double *C = (double *)calloc((long long)m * k, sizeof(double));
    if (!A || !B || !C) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    fill_random(A, m, n);
    fill_random(B, n, k);

    const double t0 = omp_get_wtime();
    multiply_omp(A, B, C, m, n, k, threads, sched, 32);
    const double t1 = omp_get_wtime();

    free(A); free(B); free(C);
    return t1 - t0;
}

int main(void)
{
    const int dims[] = {128, 256, 512, 1024, 2048};
    const size_t ndims = sizeof(dims) / sizeof(dims[0]);
    const int threads[] = {1, 2, 4, 8, 16};
    const size_t nthreads = sizeof(threads) / sizeof(threads[0]);
    const char *schedules[] = { "default", "static", "dynamic" };
    const size_t nsched = sizeof(schedules) / sizeof(schedules[0]);

    /* Seed RNG once */
    srand((unsigned)time(NULL));

    for (size_t di = 0; di < ndims; ++di) {
        int m = dims[di];
        int n = dims[di];
        int k = dims[di];
        printf("m=%d, n=%d, k=%d\n", m, n, k);
        printf("---------------------------------------------------------------\n");
        printf("%8s %9s %12s\n", "Threads", "Schedule", "Time(s)");
        printf("---------------------------------------------------------------\n");
        for (size_t si = 0; si < nsched; ++si) {
            for (size_t ti = 0; ti < nthreads; ++ti) {
                double t = run_case(m, n, k, threads[ti], schedules[si]);
                printf("%8d %9s %12.6f\n", threads[ti], schedules[si], t);
            }
        }
        printf("\n");
    }

    return 0;
}