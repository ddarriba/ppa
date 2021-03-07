/*
 * IO example 3: MPI I/O using a non-contiguous File View
 *
 * Note that, input and output files have a binary format!
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
#include <limits.h>
#include <unistd.h>
#include <mpi.h>

#include "common.h"

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  /* mpi_defs */
  int mpi_rank, mpi_size;
  MPI_File fh;
  MPI_Datatype contig_t, filetype_t;

  /* other stuff */
  int *l_mat = 0,
       lsizes[2];

  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  lsizes[0] = gsizes[0]/mpi_size;
  lsizes[1] = gsizes[1];

  /* 1. Read the input file */
  l_mat = (int *) malloc( lsizes[0] * lsizes[1] * sizeof(int) );
  MPI_File_open(MPI_COMM_WORLD, input_filename,
                MPI_MODE_RDONLY,
                MPI_INFO_NULL, &fh);

  /* We define an extended type of 2 int followed by a gap of 2 x mpi_size */
  MPI_Type_contiguous(2, MPI_INT, &contig_t);
  MPI_Type_create_resized(contig_t, 0, 2*mpi_size*sizeof(int), &filetype_t);
  MPI_Type_commit(&filetype_t);

  /* File views are interlaced, such that each process takes a chunk of 2 ints */
  MPI_File_set_view(fh, mpi_rank * 2 * sizeof(int),
                    MPI_INT, filetype_t,
                    "native", MPI_INFO_NULL);

  /* Try applying an offset here and see what happens... */
  //MPI_File_seek(fh, 1, MPI_SEEK_SET);

  MPI_File_read(fh, l_mat, lsizes[0] * lsizes[1], MPI_INT, MPI_STATUS_IGNORE);
  MPI_File_close(&fh);

  /* Print what each process have */

  for (int p=0; p<mpi_size; ++p)
  {
    MPI_Barrier(MPI_COMM_WORLD);
    if (mpi_rank != p) continue;

    printf("\nProcess %d/%d, local size =  %d x %d = %d:\n",
           mpi_rank, mpi_size, lsizes[0], lsizes[1], lsizes[0] * lsizes[1]);
    print_matrix(l_mat, lsizes[0], lsizes[1]);
  }

  free(l_mat);

  MPI_Finalize();

  return 0;
}
