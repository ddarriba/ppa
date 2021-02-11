/*
 * Example of active target synchronization using Fences
 *
 * Each process declares 2 arrays:
 *   "int * a" dynamically allocated on the heap
 *   "int b[2]" statically allocated on the stack
 * "a" positions 1 and 2 are arbitrarily initialized to "rank" and "2"
 * "b" is arbitrarily initialized to {(rank+1)*10, (rank+1)*20}
 *
 * Each process will use RMA to put its "b" values into "a" array in the
 * private memory of the next process following a ring topology
 *
 * Compile: mpicc -Wall -O3 -std=c99 -o 01_rma_fence 01_rma_fence.c
 * Run: no arguments required
 */

#include <mpi.h>
#include <stdio.h>

int main(int argc, char ** argv)
{
  int *a, b[2];
  int size, rank;
  MPI_Win win;

  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  /* create private memory */
  MPI_Alloc_mem(1000* sizeof(int), MPI_INFO_NULL, &a);
  a[0]=rank; a[1]=2; /* use private memory as usual */

  /* collectively declare memory as remotely accessible */
  MPI_Win_create(a, 1000*sizeof(int), sizeof(int),
                 MPI_INFO_NULL, MPI_COMM_WORLD,
                 &win);

  /* start access/exposure epoch */
  MPI_Win_fence(0, win);

  int target = (rank + 1) % size;
  b[0] = (rank+1) * 10;
  b[1] = (rank+1) * 20;

  MPI_Put(b,       // origin data
          2,       // origin count
          MPI_INT, // origin datatype
          target,  // target rank
          0,       // target offset
          2,       // target count
          MPI_INT, // target datatype
          win);    // window

  /* end access/exposure epoch */
  MPI_Win_fence(0, win);

  /* explicit synchronization */
  MPI_Barrier(MPI_COMM_WORLD);

  printf("Hi from %d. a[0] = %d, a[1] = %d\n", rank, a[0], a[1]);

  MPI_Win_free(&win);
  MPI_Free_mem(a);
  MPI_Finalize();
  return 0;
}
