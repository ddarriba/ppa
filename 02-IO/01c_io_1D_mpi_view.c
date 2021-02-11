/*
 * IO example 1c: MPI I/O using a File View and an offset on top of it
 *
 * This example:
 * 1. Read a matrix of integers from a file
 *
 * 2. Apply a common function to every element
 *
 * 3. Print the results to an output file
 *
 * Note that, input and output files have a binary format!
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

  /* for teaching purposes, we define 2 different views:
   *   first half of processes will view the first half of the file
   *   second half of processes will view the second half of the file
   */
  int half = mpi_rank / (mpi_size/2);

  MPI_File_set_view(fh, half * (gsizes[0] * gsizes[1] * sizeof(int) / 2),
                    MPI_INT, MPI_INT,
                    "native", MPI_INFO_NULL);

  /* afterwards, use MPI_File_seek to move into the right position.
     Note that now offset refers to 'etype' (MPI_INT) instead of bytes */
  MPI_Offset offset = (mpi_rank % (mpi_size/2)) * lsizes[0] * lsizes[1];
  MPI_File_seek(fh, offset, MPI_SEEK_SET);

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
  MPI_File_set_view(fh, mpi_rank * (lsizes[0] * lsizes[1] * sizeof(int)),
    MPI_INT, MPI_INT,
    "native", MPI_INFO_NULL);
  MPI_File_write(fh, l_mat, lsizes[0] * lsizes[1], MPI_INT, MPI_STATUS_IGNORE);
  MPI_File_close(&fh);



  free(l_mat);



  MPI_Finalize();

  return 0;
}
