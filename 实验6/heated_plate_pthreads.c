# include <stdlib.h>
# include <string.h>
# include <stdio.h>
# include <math.h>
# include <sys/time.h>

#include "parallel_for.h"

typedef struct {
  int N;
  double *old_grid;
  double *new_grid;
} PlateArgs;

/* parallel update of one interior point (4‑point stencil) */
void update_point(int idx, void *arg) {
  PlateArgs *pa = (PlateArgs *)arg;
  int N = pa->N;
  int i = idx / N;
  int j = idx % N;
  /* skip boundary cells */
  if (i == 0 || i == N - 1 || j == 0 || j == N - 1) return;
  pa->new_grid[i * N + j] =
      0.25 * (pa->old_grid[(i - 1) * N + j] +
              pa->old_grid[(i + 1) * N + j] +
              pa->old_grid[i * N + j - 1] +
              pa->old_grid[i * N + j + 1]);
}

int main ( int argc, char *argv[] );

/******************************************************************************/

int main ( int argc, char *argv[] )
{
  int thread_counts[] = {1, 2, 4, 8, 16};
  int num_options = sizeof(thread_counts) / sizeof(thread_counts[0]);
  int grid_sizes[] = {64, 128, 256, 512, 1024};
  int num_sizes = sizeof(grid_sizes) / sizeof(grid_sizes[0]);

  for (int s = 0; s < num_sizes; ++s) {
    int N = grid_sizes[s];

    for (int t = 0; t < num_options; ++t) {
      int num_threads = thread_counts[t];

      double *old_grid = malloc(N * N * sizeof(double));
      double *new_grid = malloc(N * N * sizeof(double));

      if (old_grid == NULL || new_grid == NULL) {
        printf("Memory allocation failed\n");
        return 1;
      }

      // Initialize grids
      memset(old_grid, 0, N * N * sizeof(double));
      memset(new_grid, 0, N * N * sizeof(double));

      // Set boundary values
      for (int i = 0; i < N; i++) {
        old_grid[i] = 100.0;
        new_grid[i] = 100.0;
        old_grid[(N-1)*N + i] = 100.0;
        new_grid[(N-1)*N + i] = 100.0;
        old_grid[i*N] = 0.0;
        new_grid[i*N] = 0.0;
        old_grid[i*N + N - 1] = 0.0;
        new_grid[i*N + N - 1] = 0.0;
      }

      // Initialize interior points to 0
      for (int i = 1; i < N - 1; i++) {
        for (int j = 1; j < N - 1; j++) {
          old_grid[i*N + j] = 0.0;
          new_grid[i*N + j] = 0.0;
        }
      }

      struct timeval start, end;
      gettimeofday(&start, NULL);

      double epsilon = 0.01;
      double diff;
      int iterations = 0;

      do {
        /* parallel stencil update */
        PlateArgs args = { N, old_grid, new_grid };
        parallel_for(0, N * N, 1, update_point, &args, num_threads);

        /* sequential reduction to obtain max‑difference */
        diff = 0.0;
        for (int i = 1; i < N - 1; i++) {
          for (int j = 1; j < N - 1; j++) {
            double delta = fabs(new_grid[i * N + j] - old_grid[i * N + j]);
            if (delta > diff) diff = delta;
          }
        }

        /* swap grids */
        double *temp = old_grid;
        old_grid = new_grid;
        new_grid = temp;
        iterations++;
      } while (diff > epsilon);

      gettimeofday(&end, NULL);
      double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

      double checksum = 0.0;
      for (int i = 0; i < N * N; i++) {
        checksum += old_grid[i];
      }

      printf("GridSize=%d, Threads=%d, Time=%.6f s, Checksum=%f\n", N, num_threads, elapsed, checksum);

      free(old_grid);
      free(new_grid);
    }
  }

  return 0;
}
