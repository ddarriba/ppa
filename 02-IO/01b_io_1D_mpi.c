/*
 * IO example 1b: Simple MPI I/O using Seek function
 *
 * This example:
 * 1. Read a matrix of integers from a file
 *
 * 2. Apply a common function to every element
 *
 * 3. Print the results to an output file
 *
 * Note that, input and output files have a binary format!
 *
 * Compile: mpicc -Wall -o 01b_io_1D_mpi 01b_io_1D_mpi.c common.c
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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <mpi.h>

#include "common.h"

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  /* mpi_defs */
  int mpi_rank, mpi_size;
  MPI_File fh;

  /* other stuff */
  int *l_mat = 0, /* local matrix */
       lsizes[2]; /* local sizes */

  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  lsizes[0] = gsizes[0]/mpi_size;
  lsizes[1] = gsizes[1];






  /* 1. Read the input file */
  l_mat = (int *) malloc( lsizes[0] * lsizes[1] * sizeof(int) );
  MPI_File_open(MPI_COMM_WORLD, input_filename,
                MPI_MODE_RDONLY,
                MPI_INFO_NULL, &fh);
  MPI_File_seek(fh, mpi_rank * (lsizes[0] * lsizes[1] * sizeof(int)), MPI_SEEK_SET);
  MPI_File_read(fh, l_mat, lsizes[0] * lsizes[1], MPI_INT, MPI_STATUS_IGNORE);
  MPI_File_close(&fh);











  for (int p=0; p<mpi_size; ++p)
  {
    if (mpi_rank == p)
    {
      printf("\nProcess %d/%d, local size =  %d x %d = %d:\n",
             mpi_rank, mpi_size, lsizes[0], lsizes[1], lsizes[0] * lsizes[1]);
      print_matrix(l_mat, lsizes[0], lsizes[1]);
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }

  /* 2. Apply function f to every element */
  for (int y=0; y<lsizes[0]; ++y)
    for (int x=0; x<lsizes[1]; ++x)
      l_mat[y*lsizes[1] + x] = f(l_mat[y*lsizes[1] + x]);







  /* 3. Print matrix to an output file */
  MPI_File_open(MPI_COMM_WORLD, output_filename,
                MPI_MODE_CREATE | MPI_MODE_WRONLY,
                MPI_INFO_NULL, &fh);
  MPI_File_seek(fh, mpi_rank * (lsizes[0] * lsizes[1] * sizeof(int)), MPI_SEEK_SET);
  MPI_File_write(fh, l_mat, lsizes[0] * lsizes[1], MPI_INT, MPI_STATUS_IGNORE);
  MPI_File_close(&fh);




  free(l_mat);



  MPI_Finalize();

  return 0;
}
