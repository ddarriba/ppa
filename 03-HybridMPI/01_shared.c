/*
 * Example of using MPI in shared memory
 *
 * This example performs a matrix multiplication in shared memory
 *
 * Compile: mpicc -Wall -o 01_shared 01_shared.c common.c
 * Run: No arguments required
 *
 * This file is part of the PPA distribution (https://github.com/ddarriba/ppa).
 * Copyright (c) 2021 Diego Darriba.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <mpi.h>

#include "common.h"

#define SIZE 16
#define PRINT 1

#define DIV_FACTOR 1

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
  MPI_Win b_win;


  mat_t *a, *b, *r; // A x B = R
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

  if (mpi_lsize != mpi_size)
  {
    if (!mpi_rank)
      printf("Error: This experiment is designed for shared memory only\n");
    MPI_Finalize();
    return ERROR_CONF;
  }

  if (SIZE % mpi_size)
  {
    if (!mpi_rank)
      printf("Error: size (%d) should be a multiple of the number of processes (%d)\n",
             SIZE, mpi_size);
    MPI_Finalize();
    return ERROR_DIM;
  }

  lsize = SIZE / mpi_size;
  a_start = mpi_rank * lsize * SIZE;
  b_start = mpi_rank * lsize;
  r_start = mpi_rank * lsize;

  printf("I am local process %d/%d. A start (%d), B start (%d) R start (%d)\n",
         mpi_lrank, mpi_lsize, a_start, b_start, r_start);

  a = (mat_t *) malloc (lsize * SIZE * sizeof(mat_t));
  r = (mat_t *) calloc (lsize * SIZE, sizeof(mat_t));
  MPI_Win_allocate_shared (lsize * SIZE * sizeof(mat_t), sizeof(mat_t), MPI_INFO_NULL,
                           shared_comm, &b, &b_win);

  MPI_Win_lock_all(MPI_MODE_NOCHECK, b_win);

  /* copy data to local part of shared memory */
  for (int x=0; x<SIZE; ++x)
    for (int y=0; y<lsize; ++y)
      b[x*lsize + y] = (b_start + x*SIZE + y) / DIV_FACTOR;

  MPI_Win_sync(b_win);

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
  for (int rshift = 0; rshift < mpi_size; ++rshift)
  {
    int next_rank = (mpi_rank + rshift) % mpi_size;
    MPI_Aint size;
    int disp;
    mat_t *b_ptr;
    MPI_Win_shared_query(b_win, next_rank, &size, &disp, &b_ptr);

    r_start = next_rank * lsize;
    for (int i=0; i<lsize; ++i)
      for (int j=0; j<lsize; ++j)
        for (int k=0; k<SIZE; ++k)
          r[r_start + i*SIZE + j] += a[i*SIZE + k] * b_ptr[k*lsize + j];
  }

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


  MPI_Finalize();
}
