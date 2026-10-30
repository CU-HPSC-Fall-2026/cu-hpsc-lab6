# Lab 6

- Observe differences between collectives vs point-to-point message passing
- Observe differences between blocking and non-blocking functions

## Background

The lab has several different implementations of a dot product calculation in parallel using MPI.

The computation is broken up into different phases.
1. Broadcast the pieces of the input vectors to each rank
2. Compute each ranks local contribution to the dot product
3. Reduce the local contributions into the total dot product

Each of the implementations has timers for each of these steps, with a timer running independently on each rank.

The most basic one is `mpi_dot.c`, which implements the routine using blocking 2-way point-to-point messaging.

`mpi_dot_nonblock.c` does 2-way point-to-point, but using non-blocking sends.

`mpi_dot_collectives.c` uses blocking collective operations to do the computation

`mpi_dot_collectives_alt.c` is the same as `mpi_dot_collectives.c`, but rearranged such that every rank does the exact same collective calls.
This is simply to demonstrate good MPI programming practice; whenever a collective call is made, every rank should be calling the exact same collective function in the program. This makes it clear which ranks are involved in the collective operation.

Finally, `mpi_dot_nonblock_collectives.c` uses non-blocking collective calls.

They all have command line flags for controlling the size of the vectors (`-n`) and for the number of repetitions to do (`-r`).

## Instructions

Refer to the instructions from lab-5 for building and running the files.
All the executables will be build running `make`.
Remember to run the
```console
$ module purge
$ module load gcc openmpi
```
commands every time you get into a new interactive session.

## Questions

Experiment with the example functions and observe any differences in performance between them.
You can restrict your investigation to only two ranks (though feel free to experiment with more).
Compare and contrast having ranks on the same node vs ranks on different nodes.

Here are some ideas of observations to keep track of:
- How does broadcast, reduce, and computation times compare to each other?
- What impact does non-blocking vs blocking operators have?
- What impact does point-to-point vs collective operators have?
    - Note: this impact maybe more visible when running with more than 1 rank

## Bonus
As a small extra (optional) step, see if you can turn the blocking receives in `mpi_dot_nonblock.c` into non-blocking receives.
Do you observe any significant performance changes when doing that?
What reasoning do you have for the observed change (or lack of change)?
