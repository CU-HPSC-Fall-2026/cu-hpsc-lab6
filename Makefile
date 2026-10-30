CC = mpicc
CFLAGS ?= -g -O3 -march=native -fopenmp -Wall

all: mpi_dot mpi_dot_nonblock mpi_dot_collectives mpi_dot_nonblock_collectives mpi_dot_collectives_alt
