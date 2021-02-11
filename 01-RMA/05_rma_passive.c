/*
 * Example of passive target synchronization using EXCLUSIVE locks
 * All processes start an access epoch to the root and call 2 operations
 * Root process just wait
 *
 * Each process writes its rank at the beginning of the win memory and
 * also in a dedicated section of the window. At the end, the beginning
 * of "a" will contain the rank of the process who "locked" last.
 *
 * WARNING: The lock behavior works different with MPICH and OpenMPI! 
 *
 * Compile: mpicc -Wall -O3 -std=c99 -o 05_rma_passive 05_rma_passive.c
 * Run: no arguments required
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ROOT_RANK      0
#define SIZE         128
#define MAX_ROW_SIZE  16

int main(int argc, char ** argv)
{
  int size, rank;
  MPI_Win win;

  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == ROOT_RANK)
  {
    int *a;

    /* create private memory */
    MPI_Alloc_mem(SIZE*sizeof(int), MPI_INFO_NULL, &a);
    for (int i=0; i<SIZE; ++i) a[i]=0;
    
    /* collectively declare memory as remotely accessible */
    MPI_Win_create(a, SIZE*sizeof(int), sizeof(int),
                   MPI_INFO_NULL, MPI_COMM_WORLD,
                   &win);


    MPI_Barrier(MPI_COMM_WORLD);

    for (int i=0; i<SIZE; ++i)
    {
      printf("%4d", a[i]);
      if (!((i+1)%MAX_ROW_SIZE))
        printf("\n");
    }

    MPI_Free_mem(a);
  }
  else
  {
    int *b, my_size, my_offset;

    /* declare an empty window */
    MPI_Win_create(NULL, 0, sizeof(int),
                   MPI_INFO_NULL, MPI_COMM_WORLD,
                   &win);

    my_size = SIZE/size;
    my_offset = my_size*rank;

    b = (int *)malloc(my_size * sizeof(int));
    for (int i=0; i<my_size; ++i)
      b[i] = rank;
      
    /* start an exclusive lock to root process */
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, ROOT_RANK, 0, win);

    printf("Process %d, enters lock!\n", rank);

    MPI_Put(b, my_size, MPI_INT, ROOT_RANK, 0, my_size, MPI_INT, win);
    
   /* TODO: Check that if you uncomment the line below,
      since first process reaching this point is forced to complete the
      previous Put, the other processes must wait here until the next
      Put is finished as well.
      Otherwise it breaks the atomicity of these 2 operations */

   //MPI_Win_flush_local(ROOT_RANK, win);

    printf("Process %d, put %d elements with offset %d\n",
           rank, my_size, my_offset);

   /* If flush is enforced, all processes will call the sleep sequentially,
      and you will have to wait for `my_size-1` seconds.
      Otherwise, the sleep is called (probably) at the same time */
    sleep(1);

    printf("Next, Process %d, put %d elements with value %d offset %d\n",
            rank, my_size, b[0], my_offset);

    MPI_Put(b, my_size, MPI_INT, ROOT_RANK, my_offset, my_size, MPI_INT, win);


    printf("Process %d END\n", rank);

    MPI_Win_unlock(ROOT_RANK, win);

    printf("Process %d Unlocks\n", rank);

    MPI_Barrier(MPI_COMM_WORLD);

    free(b);

  }

  MPI_Win_free(&win);
  MPI_Finalize();
  return 0;
}
