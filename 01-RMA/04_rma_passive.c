/*
 * Example of passive target synchronization
 *
 * Even processes start an access epoch to odd processes
 * Odd processes just wait
 *
 * Compile: mpicc -Wall -O3 -std=c99 -o 04_rma_passive 04_rma_passive.c
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
  a[0]=0; a[1]=0; /* use private memory as usual */

  /* collectively declare memory as remotely accessible */
  MPI_Win_create(a, 1000*sizeof(int), sizeof(int),
                 MPI_INFO_NULL, MPI_COMM_WORLD,
                 &win);

  int target = (rank + 1) % size;

  if (!(rank%2))
  {
    /* start access epoch from origin to target */
    MPI_Win_lock(MPI_LOCK_SHARED, target, 0, win);

    b[0] = (rank+1) * 10;
    b[1] = (rank+1) * 20;

    MPI_Put(b, 2, MPI_INT, target, 0, 2, MPI_INT, win);

    /* end access epoch */
    MPI_Win_unlock(target, win);
  }

  /* check how if you remove the barrier below, odd processes reach
   * the next lines before RMA finished.
   *
   * Barrier combined with the unlock at origin ensures that Put operations
   * are visible at the target
   */
  MPI_Barrier(MPI_COMM_WORLD);

  printf("Hi from %d. a[0] = %d, a[1] = %d\n", rank, a[0], a[1]);

  /* Array "a" is now accessibly by all processes in * MPI_COMM_WORLD */
  MPI_Win_free(&win);
  MPI_Free_mem(a);
  MPI_Finalize();
  return 0;
}
