#include "mpi_sync_printf.h"
#include <mpi.h>
#include <math.h>
#include "parse.h"
#include "rdtsc.h"
#include <stdio.h>
#include <stdlib.h>

#define ROOT_RANK 0

__attribute((noinline))
static double dot_opt(size_t n, const double *a, const double *b) {
  double sum = 0;
  #pragma omp simd reduction(+:sum)
  for (size_t i=0; i<n; i++)
    sum += a[i] * b[i];
  return sum;
}

int main(int argc, char *argv[]) {
  int myrank, num_ranks;
  struct Args args;
  struct argp argp = {options, parse_opt, NULL, NULL};
  argp_parse(&argp, argc, argv, 0, 0, &args);
  size_t len = args.length;

  mytimer_t bcast_timer, reduce_timer, compute_timer;
  mytimer_init(&bcast_timer);
  mytimer_init(&reduce_timer);
  mytimer_init(&compute_timer);

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

  if (args.verbose && myrank == 0) {
    double time_res = get_clock_resolution();
    printf("Clock resolution is %g\n", time_res);
  }

  int my_start = myrank * len / num_ranks;
  int my_end   = (myrank+1) * (len / num_ranks);
  if (myrank == num_ranks - 1) my_end = len;
  int my_chunksize = my_end - my_start;

  double *my_a = malloc(my_chunksize * sizeof(double));
  double *my_b = malloc(my_chunksize * sizeof(double));

  double *a = NULL, *b = NULL;
  int *chunksizes = NULL, *offsets = NULL;

  if (myrank == ROOT_RANK) {
    // Initialize
    a = malloc(len * sizeof(double));
    b = malloc(len * sizeof(double));
    for (size_t i=0; i<len; i++) {
      a[i] = 2.*(i+1);
      b[i] = 1./(i+1);
    }

    chunksizes = malloc(num_ranks * sizeof(int));
    offsets    = malloc(num_ranks * sizeof(int));
    for (int dest_rank = 0; dest_rank < num_ranks; dest_rank++) {
      int dest_start = dest_rank * len / num_ranks;
      int dest_end   = (dest_rank+1) * (len / num_ranks);
      if (dest_rank == num_ranks - 1) dest_end = len;

      chunksizes[dest_rank] = dest_end - dest_start;
      offsets[dest_rank] = dest_start;
    }
  }

  for (int rep = 0; rep < args.nreps; rep++) {
    mytimer_start(&bcast_timer);
    MPI_Scatterv(a, chunksizes, offsets, MPI_DOUBLE, my_a, my_chunksize, MPI_DOUBLE, ROOT_RANK, MPI_COMM_WORLD);
    MPI_Scatterv(b, chunksizes, offsets, MPI_DOUBLE, my_b, my_chunksize, MPI_DOUBLE, ROOT_RANK, MPI_COMM_WORLD);
    mytimer_end(&bcast_timer);

    mytimer_start(&compute_timer);
    double my_dot = dot_opt(my_chunksize, my_a, my_b);
    mytimer_end(&compute_timer);

    double dot = 0;
    mytimer_start(&reduce_timer);
    MPI_Reduce(&my_dot, &dot, 1, MPI_DOUBLE, MPI_SUM, ROOT_RANK, MPI_COMM_WORLD);
    mytimer_end(&reduce_timer);

    if (myrank == ROOT_RANK && fabs(dot - len*2) > 1e-10) {
      printf("WRONG RESULT! dot = %e, but correct value is %ld\n", dot, len*2);
    }

  } // End of repetitions

  if (myrank == ROOT_RANK) {
    free(a);
    free(b);
    free(chunksizes);
    free(offsets);
  }

  mpi_sync_printf(MPI_COMM_WORLD, "bcast_time: %.4E, reduce_time: %.4E, compute_time: %.4E", bcast_timer.total / args.nreps, reduce_timer.total / args.nreps, compute_timer.total / args.nreps);

  free(my_a); free(my_b);
  MPI_Finalize();
}
