#include <stdlib.h>
#include <stdbool.h>
#include <argp.h>

// -- Command line argument parsing
struct Args {
  size_t length;
  size_t nreps;
  bool block;
  long verbose;
};

static struct argp_option options[] = {
  {"length", 'n', "size_t", 0, "Length of each vector"},
  {"nreps", 'r', "size_t", 0, "Number of repetitions"},
  {"block", 'b', NULL, 0, "Compute block dot products (versus a single dot product)"},
  {"verbose", 'v', "(0|1)", 0, "Whether to print the resolution and header information (for easier data extraction)"},
};

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  struct Args *args = (struct Args *)state->input;
  switch (key) {
  case ARGP_KEY_INIT:
    args->length = 100;
    args->nreps = 10;
    args->block = false;
    args->verbose = 1;
    break;
  case 'n':
    args->length = strtol(arg, NULL, 10);
    break;
  case 'r':
    args->nreps = strtol(arg, NULL, 10);
    break;
  case 'b':
    args->block = true;
    break;
  case 'v':
    args->verbose = strtol(arg, NULL, 10);
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
