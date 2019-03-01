#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <mpi.h>

#include "common.h"

#define SIZE 4
#define DIV_FACTOR 1
#define PRINT 1

void sleep_barrier( MPI_Comm comm )
{
  fflush(stdout);
  usleep(100);
  MPI_Barrier(comm);
}

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  /* mpi_defs */
  int mpi_rank, mpi_size,   // global size/rank
      mpi_lrank, mpi_lsize; // local size/rank
  MPI_Comm shared_comm;
  MPI_Win b_win, b_remote_win;

  mat_t *a, *b, *b_rma, *r; // A x B = R
  int lsize;
  int a_start, b_start, r_start;

  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  MPI_Comm_split_type(MPI_COMM_WORLD,       // comm to split
                      MPI_COMM_TYPE_SHARED, // split type
                      mpi_rank,             // key for ranking
                      MPI_INFO_NULL,        // info object
                      &shared_comm);        // output comm

  MPI_Comm_rank(shared_comm, &mpi_lrank);
  MPI_Comm_size(shared_comm, &mpi_lsize);

  if (SIZE % mpi_size)
  {
    if (!mpi_rank)
      printf("Error: size (%d) should be a multiple of the number of processes (%d)\n",
             SIZE, mpi_size);
    MPI_Finalize();
    return ERROR_DIM;
  }

  printf("I am local process %d/%d and global process %d/%d\n",
         mpi_lrank, mpi_lsize, mpi_rank, mpi_size);

  lsize = SIZE / mpi_size;
  a_start = mpi_rank * lsize * SIZE;
  b_start = mpi_rank * lsize;
  r_start = mpi_rank * lsize;

  a = (mat_t *) malloc (lsize * SIZE * sizeof(mat_t));
  r = (mat_t *) calloc (lsize * SIZE, sizeof(mat_t));

  /* NOTE that window below is defined with the shared communicator,
   * and thus is not accessible from processes in a different node */
  MPI_Win_allocate_shared (lsize * SIZE * sizeof(mat_t), sizeof(mat_t), MPI_INFO_NULL,
                           shared_comm, &b, &b_win);

  MPI_Win_allocate (lsize * SIZE * sizeof(mat_t), sizeof(mat_t), MPI_INFO_NULL,
                    MPI_COMM_WORLD, &b_rma, &b_remote_win);

  MPI_Win_lock_all(MPI_MODE_NOCHECK, b_win);
  //MPI_Win_lock_all(MPI_MODE_NOCHECK, b_remote_win);


  MPI_Group world_group, shared_group;
  int map0[mpi_size], map1[mpi_size];
  for (int i=0; i<mpi_size; i++)
    map0[i] = i;

  MPI_Comm_group (MPI_COMM_WORLD, &world_group);
  MPI_Comm_group (shared_comm, &shared_group);
  MPI_Group_translate_ranks (world_group, mpi_size, map0, shared_group, map1);

  for (int p=0; p<mpi_size; ++p)
  {
    if (p == mpi_rank)
    {
      for (int i=0; i<mpi_size; i++)
        printf("Global %d mapped to local %d\n",
          map0[i], map1[i]);
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }

  /* copy data to local part of shared memory */
  for (int x=0; x<SIZE; ++x)
    for (int y=0; y<lsize; ++y)
      b[x*lsize + y] = (b_start + x*SIZE + y) / DIV_FACTOR;

  MPI_Win_sync(b_win);

  MPI_Win_fence(0, b_remote_win);

  memcpy(b_rma, b, lsize * SIZE * sizeof(mat_t));

  MPI_Win_fence(0, b_remote_win);

  for (int y=0; y<lsize; ++y)
    for (int x=0; x<SIZE; ++x)
      a[y*SIZE + x] = (a_start + y*SIZE + x) / DIV_FACTOR;

#if(PRINT)
  for (int p=0; p<mpi_size; ++p)
  {
    if (mpi_rank == p)
    {
      printf("\nMatrix A Process {%d}, local size =  %d x %d = %d:\n",
             mpi_rank, lsize, SIZE, lsize * SIZE);
      print_matrix(a, lsize, SIZE);
    }
    sleep_barrier(shared_comm);
  }

  for (int p=0; p<mpi_size; ++p)
  {
    if (mpi_rank == p)
    {
      printf("\nMatrix B Process {%d}, local size =  %d x %d = %d:\n",
             mpi_rank, lsize, SIZE, lsize * SIZE);
      print_matrix(b, SIZE, lsize);
    }
    sleep_barrier(shared_comm);
  }
#endif

  /* Note that a process can also query itself! */
  mat_t *rbuf = (mat_t *) malloc (lsize * SIZE * sizeof(mat_t));
  for (int rshift = 0; rshift < mpi_size; ++rshift)
  {
    int next_rank = (mpi_rank + rshift) % mpi_size;
    MPI_Aint size;
    int disp;
    mat_t *b_ptr;

    if (map1[next_rank] != MPI_UNDEFINED)
    {
      MPI_Win_shared_query(b_win, map1[next_rank], &size, &disp, &b_ptr);
    }
    else
    {
      MPI_Get(rbuf, lsize * SIZE, MPI_FLOAT, next_rank,
        0, lsize * SIZE, MPI_FLOAT, b_remote_win);
      MPI_Win_fence(0, b_remote_win);
      b_ptr = rbuf;
    }

    r_start = next_rank * lsize;
    for (int i=0; i<lsize; ++i)
      for (int j=0; j<lsize; ++j)
        for (int k=0; k<SIZE; ++k)
          r[r_start + i*SIZE + j] += a[i*SIZE + k] * b_ptr[k*lsize + j];
  }
  free(rbuf);

#if (PRINT)
  sleep_barrier(shared_comm);

  for (int p=0; p<mpi_size; ++p)
  {
    if (mpi_rank == p)
    {
      if (!mpi_rank)
        printf("\n\nResult matrix:\n");
      print_matrix(r, lsize, SIZE);
    }
    sleep_barrier(shared_comm);
  }
#endif

  free(a);
  free(r);
  MPI_Win_unlock_all(b_win);
  MPI_Win_free (&b_win);
  MPI_Win_free (&b_remote_win);

  MPI_Finalize();
}
