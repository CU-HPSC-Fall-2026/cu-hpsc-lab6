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
  double *a = NULL, *b = NULL; // these are only used by the master rank, so best practice to initialize to NULL
  double *my_a = NULL, *my_b = NULL;

  int tag1 = 1, tag2 = 2, tag3 = 3;

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

  if (myrank == ROOT_RANK) {
    a = malloc(len * sizeof(double));
    b = malloc(len * sizeof(double));
    for (size_t i=0; i<len; i++) {
      a[i] = 2.*(i+1);
      b[i] = 1./(i+1);
    }
  }

  for (int rep = 0; rep < args.nreps; rep++) {
    int my_chunksize;

    if (myrank == ROOT_RANK) {
      for (int dest_rank = 1; dest_rank < num_ranks; dest_rank++) {
        int dest_start = dest_rank * len / num_ranks;
        int dest_end   = (dest_rank+1) * (len / num_ranks);
        if (dest_rank == num_ranks - 1) dest_end = len;

        int chunksize = dest_end - dest_start;

        mytimer_start(&bcast_timer);
        MPI_Send(&chunksize, 1, MPI_INT, dest_rank, tag1, MPI_COMM_WORLD);
        MPI_Send(&a[dest_start], chunksize, MPI_DOUBLE, dest_rank, tag2, MPI_COMM_WORLD);
        MPI_Send(&b[dest_start], chunksize, MPI_DOUBLE, dest_rank, tag3, MPI_COMM_WORLD);
        mytimer_end(&bcast_timer);
      }
      // We have the master rank operate on the original array for simplicities sake
      my_a = a;
      my_b = b;

      int start = myrank * len / num_ranks;
      int end   = (myrank+1) * (len / num_ranks);
      my_chunksize = end - start;
    } else {
      int source_rank = ROOT_RANK;
      MPI_Status status;

      mytimer_start(&bcast_timer);
      MPI_Recv(&my_chunksize, 1, MPI_INT, source_rank, tag1, MPI_COMM_WORLD, &status);
      mytimer_end(&bcast_timer);
      my_a = malloc(my_chunksize * sizeof(double));
      my_b = malloc(my_chunksize * sizeof(double));

      mytimer_start(&bcast_timer);
      MPI_Recv(my_a, my_chunksize, MPI_DOUBLE, source_rank, tag2, MPI_COMM_WORLD, &status);
      MPI_Recv(my_b, my_chunksize, MPI_DOUBLE, source_rank, tag3, MPI_COMM_WORLD, &status);
      mytimer_end(&bcast_timer);
    }

    mytimer_start(&compute_timer);
    double my_dot = dot_opt(my_chunksize, my_a, my_b);
    mytimer_end(&compute_timer);

    double dot = 0;
    mytimer_start(&reduce_timer);
    if (myrank == ROOT_RANK) {
      MPI_Status status;
      dot = my_dot;
      for (int source_rank = 1; source_rank < num_ranks; source_rank++) {
        double source_dot;
        MPI_Recv(&source_dot, 1, MPI_DOUBLE, source_rank, tag1, MPI_COMM_WORLD, &status);
        dot += source_dot;
      }
    } else {
      int dest_rank = ROOT_RANK;
      MPI_Send(&my_dot, 1, MPI_DOUBLE, dest_rank, tag1, MPI_COMM_WORLD);
    }
    mytimer_end(&reduce_timer);

    if (myrank == ROOT_RANK && fabs(dot - len*2) > 1e-10) {
      printf("WRONG RESULT! dot = %e, but correct value is %ld\n", dot, len*2);
    }
  }

  mpi_sync_printf(MPI_COMM_WORLD, "bcast_time: %.4E, reduce_time: %.4E, compute_time: %.4E", bcast_timer.total / args.nreps, reduce_timer.total / args.nreps, compute_timer.total / args.nreps);

  if (myrank == ROOT_RANK) {
    free(a); free(b);
  } else {
    free(my_a); free(my_b);
  }
  MPI_Finalize();
}
