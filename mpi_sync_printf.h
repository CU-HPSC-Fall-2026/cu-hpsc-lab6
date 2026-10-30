#include "mpi.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// Gathers print messages from all ranks on `comm`, prepends the rank id to the message, then prints them in rank order
void mpi_sync_printf(MPI_Comm comm, const char format[], ...) {
  int my_rank, num_ranks;
  char *prepended_string;
  va_list Argp;

  MPI_Comm_rank(comm, &my_rank);
  MPI_Comm_size(comm, &num_ranks);

  { // Get this rank's print message
    char *orig_string;
    va_start(Argp, format);
    vasprintf(&orig_string, format, Argp);
    va_end(Argp);

    asprintf(&prepended_string, "[%d] %s", my_rank, orig_string);
    free(orig_string);
  }

  int my_strlen = strlen(prepended_string) + 1; // include NULL terminator

  int *strlens = NULL, *offsets = NULL;
  char *all_strings = NULL;

  if (my_rank == 0) {
    strlens = (int *)malloc(num_ranks * sizeof(int));
    offsets = (int *)malloc(num_ranks * sizeof(int));
  }

  MPI_Gather(&my_strlen, 1, MPI_INT, strlens, 1, MPI_INT, 0, comm);

  if (my_rank == 0) {
    int total_length = 0;
    for (int rank = 0; rank < num_ranks; rank++) {
      offsets[rank] = total_length;
      total_length += strlens[rank];
    }
    all_strings = (char *)malloc(total_length * sizeof(char));
  }

  MPI_Gatherv(prepended_string, my_strlen, MPI_CHAR, all_strings, strlens, offsets, MPI_CHAR, 0, comm);

  if (my_rank == 0) {
    for (int rank = 0; rank < num_ranks; rank++) {
      puts(&all_strings[offsets[rank]]);
    }
    free(all_strings);
    free(strlens);
    free(offsets);
  }
  free(prepended_string);
}
