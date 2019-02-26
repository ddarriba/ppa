/*
 * IO example 3: MPI I/O using Seek function
 *
 * This example:
 * 1. Read a matrix of integers from a file
 * 2. Apply a common function to every element
 * 3. Print the results to an output file
 *
 * Note that, input and output files have a binary format!
 */
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <mpi.h>

#define DEFAULT_INPUT_FN  "data/integers.input"
#define DEFAULT_OUTPUT_FN "integers.output"

int f(int x)
{
  /* perform an integer division */
  /* hence for default input (0,1,2,3,...), 
                       output should be (0,0,1,1,2,2,...) */
  return x/2;
}

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  /* input parameters */
  char * input_filename  = (argc == 5)?argv[1]:DEFAULT_INPUT_FN,
       * output_filename = (argc == 5)?argv[2]:DEFAULT_OUTPUT_FN;
           
  int h = (argc == 5)?atoi(argv[3]):16;
  int w = (argc == 5)?atoi(argv[4]):16;

  /* mpi_defs */
  int mpi_rank, mpi_size;
  MPI_File fh;

  /* other stuff */
  int *l_mat = 0,
       l_h,
       l_w;

  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  l_h = h/mpi_size;
  l_w = w;

  /* 1. Read the input file */
  l_mat = (int *) malloc( l_h * l_w * sizeof(int) );
  MPI_File_open(MPI_COMM_WORLD, input_filename,
                MPI_MODE_RDONLY,
                MPI_INFO_NULL, &fh);
  MPI_File_seek(fh, mpi_rank * (l_h * l_w * sizeof(int)), MPI_SEEK_SET);
  MPI_File_read(fh, l_mat, l_h * l_w, MPI_INT, MPI_STATUS_IGNORE);
  MPI_File_close(&fh);

  /* 2. Apply function f to every element */
  for (int y=0; y<l_h; ++y)
    for (int x=0; x<l_w; ++x)
      l_mat[y*l_w + x] = f(l_mat[y*l_w + x]);

  for (int p=0; p<mpi_size; ++p)
  {
    MPI_Barrier(MPI_COMM_WORLD);
    if (mpi_rank != p) continue;

    printf("\nProcess %d/%d, local size =  %d x %d = %d:\n", 
           mpi_rank, mpi_size, l_h, l_w, l_h * l_w);

    for (int y=0; y<l_h; ++y)
    {
      for (int x=0; x<l_w; ++x)
        printf("%4d ", l_mat[y*l_w + x]);
      printf("\n");
    }
  }

  /* 3. Print matrix to an output file */
  MPI_File_open(MPI_COMM_WORLD, output_filename,
                MPI_MODE_CREATE | MPI_MODE_WRONLY,
                MPI_INFO_NULL, &fh);
  MPI_File_seek(fh, mpi_rank * (l_h * l_w * sizeof(int)), MPI_SEEK_SET);
  MPI_File_write(fh, l_mat, l_h * l_w, MPI_INT, MPI_STATUS_IGNORE);
  MPI_File_close(&fh);

  free(l_mat);

  MPI_Finalize();

  return 0;
}
