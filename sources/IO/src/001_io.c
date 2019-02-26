/*
 * IO example 1: Centralized I/O
 *
 * This example:
 * 1. Read a matrix of integers from a file
 * 2. Scatter it among all processes
 * 3. Apply a common function to every element
 * 4. Gather the matrix back
 * 5. Print the results to an output file
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

  /* other stuff */
  int *mat = 0, 
      *l_mat = 0,
       l_h,
       l_w;

  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  l_h = h/mpi_size;
  l_w = w;

  /* 1. Root process reads the input file */
  if (!mpi_rank)
  {
    FILE * fd;

    mat = (int *) malloc( h * w * sizeof(int) );
    fd = fopen(input_filename, "r");
    if (fread(mat, sizeof(int), h*w, fd) != h*w)
    {
      printf("Error reading input file or dimensions are wrong\n");
    }
    fclose(fd);
  }

  /* 2. Data is scattered among all processes */
  l_mat = (int *) malloc( l_h * l_w * sizeof(int) );
  MPI_Scatter(mat,   l_h * l_w, MPI_INT,
              l_mat, l_h * l_w, MPI_INT,
              0, MPI_COMM_WORLD);

  /* 3. Apply function f to every element */
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

  /* 4. Gather results back to root */
  MPI_Gather(l_mat, l_h * l_w, MPI_INT,
             mat,   l_h * l_w, MPI_INT,
             0, MPI_COMM_WORLD);

  /* 5. Print matrix to an output file */
  if (!mpi_rank)
  {
    FILE * fd = fopen(output_filename, "w");
    if (fwrite(mat, sizeof(int), h*w, fd) != h*w)
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
