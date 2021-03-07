/*
 * Example of using communicators in hybrid distributed/shared memory
 *
 * Prints local and global ranks
 *
 * Compile: mpicc -Wall -o 02_hybrid_comm 02_hybrid_comm.c
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

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  /* mpi_defs */
  int mpi_rank, mpi_size,   // global size/rank
      mpi_lrank, mpi_lsize; // local size/rank
  MPI_Comm shared_comm;

  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);


  MPI_Comm_split_type(MPI_COMM_WORLD,       // comm to split
                      MPI_COMM_TYPE_SHARED, // split type
                      mpi_rank,             // key for ranking
                      MPI_INFO_NULL,        // info object
                      &shared_comm);        // output comm

  MPI_Comm_rank(shared_comm, &mpi_lrank);
  MPI_Comm_size(shared_comm, &mpi_lsize);


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
      printf("I am local process %d/%d and global process %d/%d\n",
             mpi_lrank, mpi_lsize, mpi_rank, mpi_size);
      for (int i=0; i<mpi_size; i++)
        printf("[%d] Global %d mapped to local %d\n",
          mpi_rank, map0[i], map1[i]);
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }

  MPI_Finalize();
}
