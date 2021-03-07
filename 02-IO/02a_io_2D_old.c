/*
 * IO example 2a: Simple centralized 2D I/O
 * using derived datatypes and virtual topology
 *
 * Compile: mpicc -Wall -o 02a_io_2D_old 02a_io_2D_old.c common.c
 *
 * Run: mpirun -np N 02a_io_2D_old Sx Sy
 *      Sx times Sy must equal N
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
#include <unistd.h>
#include <mpi.h>

#include "common.h"

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  /* mpi_defs */
  int mpi_rank, mpi_size,
      psizes[2],            /* processes per dim */
      periods[2] = {1,1},   /* periodic grid */
      grid_rank,            /* local rank in grid */
      grid_coord[2];        /* process coordinates in grid */
  MPI_Comm grid_comm;       /* grid communicator */
  MPI_Datatype contig_t, colblock_t;

  /* other stuff */
  int lsizes[2];            /* local sizes */
  int *mat = 0,
      *l_mat = 0;

  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  if (argc != 3)
  {
    if (!mpi_rank)
      printf("Usage: %s procs_y procs_x\n", argv[0]);
    MPI_Finalize();
    exit(ERROR_ARGS);
  }

  psizes[0] = atoi(argv[1]);
  psizes[1] = atoi(argv[2]);

  if (psizes[0] * psizes[1] != mpi_size)
  {
    if (!mpi_rank)
      printf("Error: procs_y (%d) x procs_x (%d) != mpi_size (%d)\n",
             psizes[0], psizes[1], mpi_size);
    MPI_Finalize();
    exit(ERROR_DIM);
  }

  lsizes[0] = gsizes[0] / psizes[0];
  lsizes[1] = gsizes[1] / psizes[1];

  if (!mpi_rank)
  {
    printf("Grid for %d processes in %d by %d grid\n",
           mpi_size, psizes[0], psizes[1]);
    printf("New datatype of %d elements with %d padding\n",
           lsizes[1], gsizes[1]);
  }
  MPI_Barrier(MPI_COMM_WORLD);

  MPI_Cart_create(MPI_COMM_WORLD, 2, psizes, periods, 0, &grid_comm);
  MPI_Comm_rank(grid_comm, &grid_rank);
  MPI_Cart_coords(grid_comm, grid_rank, 2, grid_coord);


  MPI_Type_vector(lsizes[0], lsizes[1], gsizes[1], MPI_INT, &contig_t);
  MPI_Type_create_resized(contig_t,     /* input datatype */
                          0,            /* new lower bound */
                          sizeof(int),  /* new extent */
                          &colblock_t); /* new datatype (output) */
  MPI_Type_commit(&colblock_t);

  /* 1. Root process reads the input file */
  if (!mpi_rank)
  {
    FILE * fd;

    mat = (int *) malloc( gsizes[0] * gsizes[1] * sizeof(int) );
    fd = fopen(input_filename, "r");
    if (fread(mat, sizeof(int), gsizes[0] * gsizes[1], fd) != gsizes[0]*gsizes[1])
    {
      printf("Error reading input file or dimensions are wrong");
    }
    fclose(fd);
  }

  /* 2. Data is scattered among all processes */

    int disps[psizes[0]*psizes[1]];
    int counts[psizes[0]*psizes[1]];
    for (int y=0; y<psizes[0]; y++) {
        for (int x=0; x<psizes[1]; x++) {
            disps[y*psizes[1]+x] = y*gsizes[1]*lsizes[0]+x*lsizes[1];
            counts [y*psizes[1]+x] = 1;
        }
    }

  l_mat = (int *) malloc( lsizes[0] * lsizes[1] * sizeof(int) );
  MPI_Scatterv(mat,   counts, disps, colblock_t,
              l_mat, lsizes[0] * lsizes[1], MPI_INT,
              0, grid_comm);

  for (int p=0; p<mpi_size; ++p)
  {
    if (mpi_rank == p)
    {
      printf("\nProcess {%d,%d}, local size =  %d x %d = %d:\n",
             grid_coord[0], grid_coord[1], lsizes[0], lsizes[1], lsizes[0] * lsizes[1]);
      print_matrix(l_mat, lsizes[0], lsizes[1]);
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }

  /* 3. Apply function f to every element */
  for (int y=0; y<lsizes[0]; ++y)
    for (int x=0; x<lsizes[1]; ++x)
      l_mat[y*lsizes[1] + x] = f(l_mat[y*lsizes[1] + x]);

  /* 4. Gather results back to root */
  MPI_Gatherv(l_mat, lsizes[0] * lsizes[1], MPI_INT,
              mat, counts, disps, colblock_t,
              0, grid_comm);

  /* 5. Print matrix to an output file */
  if (!mpi_rank)
  {
    /* print gathered matrix */
    printf("\nGathered output matrix:\n");
    print_matrix(mat, gsizes[0], gsizes[1]);

    FILE * fd = fopen(output_filename, "w");
    if (fwrite(mat, sizeof(int), gsizes[0] * gsizes[1], fd) != gsizes[0]*gsizes[1])
    {
      printf("Error writing output file or dimensions are wrong\n");
    }
    fclose(fd);
  }

  free(l_mat);
  if (!mpi_rank)
    free(mat);

  MPI_Finalize();

  return 0;
}
