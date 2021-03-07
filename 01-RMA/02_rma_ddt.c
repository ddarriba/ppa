/*
 * Example of how to use derived datatypes with RMA operations
 *
 * Accessing concurrently a window using a column datatype
 *
 * Compile: mpicc -Wall -O3 -std=c99 -o 02_rma_ddt 02_rma_ddt.c
 * Run: no arguments required
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
#include <mpi.h>
#include <stdio.h>

#define ROWS 10
#define COLS 16

int main(int argc, char ** argv)
{
  int *a, b[ROWS];
  int size, rank;
  MPI_Win win;
  MPI_Datatype contig_t, col_t;

  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  MPI_Type_vector(ROWS, 1, COLS, MPI_INT, &contig_t);
  MPI_Type_create_resized(contig_t,     /* input datatype */
                          0,            /* new lower bound */
                          sizeof(int),  /* new extent */
                          &col_t); /* new datatype (output) */
  MPI_Type_commit(&col_t);

  /* create private memory */
  MPI_Alloc_mem(ROWS * COLS * sizeof(int), MPI_INFO_NULL, &a);
  for (int i=0; i<ROWS * COLS; ++i) a[i] = 0;

  /* collectively declare memory as remotely accessible */
  MPI_Win_create(a, 1000*sizeof(int), sizeof(int),
                 MPI_INFO_NULL, MPI_COMM_WORLD,
                 &win);

  /* start access/exposure epoch */
  MPI_Win_fence(0, win);

  for (int i=0; i<ROWS; ++i)
    b[i] = (rank+1) * 10 + i;

  MPI_Put(b,       // source
          ROWS,    // origin count
          MPI_INT, // origin datatype
          0,       // target (root)
          rank,    // target offset
          1,       // target count
          col_t,   // target datatype
          win);    // window

  /* end access/exposure epoch */
  MPI_Win_fence(0, win);

  if (!rank)
  {
    printf("Hi from root of %d processes\n", size);
    for (int y=0; y<ROWS; ++y)
    {
      printf("  ");
      for (int x=0; x<COLS; ++x)
      {
        printf("%2d ", a[y*COLS+x]);
      }
      printf("\n");
    }
  }

  MPI_Win_free(&win);
  MPI_Free_mem(a);
  MPI_Finalize();
  return 0;
}
