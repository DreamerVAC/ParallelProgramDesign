# include <stdlib.h>
# include <stdio.h>
# include <math.h>
# include <omp.h>

int thread_counts[] = {1, 2, 4, 8, 16};
int sizes[]         = {64, 128, 256, 512, 1024};
int num_threads     = sizeof(thread_counts) / sizeof(thread_counts[0]);
int num_sizes       = sizeof(sizes) / sizeof(sizes[0]);
#define IDX(i,j) ((i)*N + (j))

int main ( int argc, char *argv[] )
{
  double *u, *w;
  int M, N;

  for (int si = 0; si < num_sizes; si++) {
      M = N = sizes[si];
      for (int ti = 0; ti < num_threads; ti++) {
          int threads = thread_counts[ti];
          omp_set_num_threads(threads);

          u = malloc(sizeof(double) * M * N);
          w = malloc(sizeof(double) * M * N);
          double mean = 0.0, diff = 0.0, my_diff;
          int iterations = 0, iterations_print = 1;
          double wtime;

          // Initialize and set boundaries (convert all w[i][j] to w[IDX(i,j)])
#pragma omp parallel for
          for (int i = 1; i < M-1; i++) w[IDX(i,0)] = 100.0;
#pragma omp parallel for
          for (int i = 1; i < M-1; i++) w[IDX(i,N-1)] = 100.0;
#pragma omp parallel for
          for (int j = 0; j < N; j++) w[IDX(M-1,j)] = 100.0;
#pragma omp parallel for
          for (int j = 0; j < N; j++) w[IDX(0,j)] = 0.0;

          // Compute initial mean
#pragma omp parallel for reduction(+ : mean)
          for (int i = 1; i < M-1; i++) mean += w[IDX(i,0)] + w[IDX(i,N-1)];
#pragma omp parallel for reduction(+ : mean)
          for (int j = 0; j < N; j++) mean += w[IDX(0,j)] + w[IDX(M-1,j)];
          mean /= (2.0 * M + 2.0 * N - 4.0);

          // Initialize interior
#pragma omp parallel for collapse(2)
          for (int i = 1; i < M-1; i++)
              for (int j = 1; j < N-1; j++)
                  w[IDX(i,j)] = mean;

          // Timing start
          wtime = omp_get_wtime();

          // Iteration loop
          diff = mean; // ensure at least one iteration
          while (diff >= 0.001) {
              // copy w to u
#pragma omp parallel for collapse(2)
              for (int i = 0; i < M; i++)
                  for (int j = 0; j < N; j++)
                      u[IDX(i,j)] = w[IDX(i,j)];

              // update w
#pragma omp parallel for collapse(2)
              for (int i = 1; i < M-1; i++)
                  for (int j = 1; j < N-1; j++)
                      w[IDX(i,j)] = 0.25*(u[IDX(i-1,j)] + u[IDX(i+1,j)] + u[IDX(i,j-1)] + u[IDX(i,j+1)]);

              // compute max diff
              diff = 0.0;
#pragma omp parallel for collapse(2) private(my_diff) reduction(max : diff)
              for (int i = 1; i < M-1; i++)
                  for (int j = 1; j < N-1; j++) {
                      my_diff = fabs(w[IDX(i,j)] - u[IDX(i,j)]);
                      if (my_diff > diff) diff = my_diff;
                  }
              iterations++;
          }

          wtime = omp_get_wtime() - wtime;
          printf("Size=%d, Threads=%d, Time=%f\n", M, threads, wtime);

          free(u);
          free(w);
      }
  }
  return 0;
}
