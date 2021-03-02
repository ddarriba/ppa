/*
 * IO example 1a: Simple centralized I/O
 *
 * This example:
 * 1. Read a matrix of integers from a file
 * 2. Scatter it among all processes
 * 3. Apply a common function to every element
 * 4. Gather the matrix back
 * 5. Print the results to an output file
 *
 * Note that, input and output files have a binary format!
 *
 * Compile: mpicc -Wall -o 01a_io_1D_old 01a_io_1D_old.c common.c
 * Run: no arguments required
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

  /* other stuff */
  int *mat = 0,   /* global matrix */
      *l_mat = 0, /* local matrix */
       lsizes[2]; /* local sizes */

  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  lsizes[0] = gsizes[0]/mpi_size;
  lsizes[1] = gsizes[1];

  /* 1. Root process reads the input file */
  if (!mpi_rank)
  {
    FILE * fd;

    mat = (int *) malloc( gsizes[0] * gsizes[1] * sizeof(int) );
    fd = fopen(input_filename, "r");
    if (fread(mat, sizeof(int), gsizes[0]*gsizes[1], fd) != gsizes[0]*gsizes[1])
    {
      printf("Error reading input file or dimensions are wrong\n");
    }
    fclose(fd);
  }

  /* 2. Data is scattered among all processes */
  l_mat = (int *) malloc( lsizes[0] * lsizes[1] * sizeof(int) );
  MPI_Scatter(mat,   lsizes[0] * lsizes[1], MPI_INT,
              l_mat, lsizes[0] * lsizes[1], MPI_INT,
              0, MPI_COMM_WORLD);





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

  /* 3. Apply function f to every element */
  for (int y=0; y<lsizes[0]; ++y)
    for (int x=0; x<lsizes[1]; ++x)
      l_mat[y*lsizes[1] + x] = f(l_mat[y*lsizes[1] + x]);


  /* 4. Gather results back to root */
  MPI_Gather(l_mat, lsizes[0] * lsizes[1], MPI_INT,
             mat,   lsizes[0] * lsizes[1], MPI_INT,
             0, MPI_COMM_WORLD);

  /* 5. Print matrix to an output file */
  if (!mpi_rank)
  {
    FILE * fd = fopen(output_filename, "w");
    if (fwrite(mat, sizeof(int), gsizes[0]*gsizes[1], fd) != gsizes[0]*gsizes[1])
    {
      printf("Error writing output file\n");
    }
    fclose(fd);
  }

  free(l_mat);
  if (!mpi_rank)
    free(mat);

  MPI_Finalize();

  return 0;
}
