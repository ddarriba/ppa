/*
 * Example of active target synchronization using PSCW
 *
 * Processes group pairwise and perform 2 steps
 * 1. Even processes start an exposure epoch
 *    Odd processes start an access epoch and perform a put opearation
 * 2. The other way around
 *
 * Compile: mpicc -Wall -O3 -std=c99 -o 03_rma_pscw 03_rma_pscw.c
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
  MPI_Group world_group, group;

  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  /* The number of processes must be even */
  if (size%2)
  {
    if (!rank)
      printf("ERROR: The number of processes must be even\n");
    MPI_Finalize();
    return 1;
  }

  MPI_Comm_group(
  	MPI_COMM_WORLD,
  	&world_group);

  /* create private memory */
  MPI_Alloc_mem(1000* sizeof(int), MPI_INFO_NULL, &a);
  a[0]=42; a[1]=1027; /* use private memory as usual */

  /* collectively declare memory as remotely accessible */
  MPI_Win_create(a, 1000*sizeof(int), sizeof(int),
                 MPI_INFO_NULL, MPI_COMM_WORLD,
                 &win);

  /* we will place our rank in our partner's memory */
  b[0] = b[1] = rank;

  if (!(rank % 2))
  {
    int other_rank = (rank + 1) % size;

    printf("Process %d groups with %d\n", rank, other_rank);
    MPI_Group_incl(world_group, 1, &other_rank, &group);

    printf("Process %d starts exposure epoch\n", rank);

    /* start exposure epoch */
    MPI_Win_post(group, 0, win);

    printf("Process %d calls end of exposure epoch\n", rank);
    MPI_Win_wait(win);

    printf("Process %d ends exposure epoch\n", rank);

    printf(">> Process %d starts access epoch\n", rank);
    /* start access epoch */
    MPI_Win_start(group, 0, win);
    printf(">> Process %d Put\n", rank);

    /* do operations */
    MPI_Put(b, 2, MPI_INT, other_rank, 0, 2, MPI_INT, win);

    printf(">> Process %d calls end of access epoch\n", rank);

    /* end access epoch */
    MPI_Win_complete(win);
    printf(">> Process %d ends access epoch\n", rank);
  }
  else
  {
    int other_rank = (rank + size - 1) % size;

    printf("Process %d groups with %d\n", rank, other_rank);
    MPI_Group_incl(world_group, 1, &other_rank, &group);

    printf("Process %d starts access epoch\n", rank);

    /* start access epoch */
    MPI_Win_start(group, 0, win);
    printf("Process %d Put\n", rank);

    /* do operations */
    MPI_Put(b, 2, MPI_INT, other_rank, 0, 2, MPI_INT, win);

    printf("Process %d calls end of access epoch\n", rank);

    /* end access epoch */
    MPI_Win_complete(win);
    printf("Process %d ends access epoch\n", rank);



    printf(">> Process %d starts exposure epoch\n", rank);

    /* start exposure epoch */
    MPI_Win_post(group, 0, win);

    printf(">> Process %d calls end of exposure epoch\n", rank);
    MPI_Win_wait(win);

    printf(">> Process %d ends exposure epoch\n", rank);
  }

  //MPI_Barrier(MPI_COMM_WORLD);

  printf("Hi from %d. a[0] = %d, a[1] = %d\n", rank, a[0], a[1]);

  /* Array "a" is now accessibly by all processes in * MPI_COMM_WORLD */
  MPI_Win_free(&win);
  MPI_Free_mem(a);
  MPI_Finalize();
  return 0;
}
