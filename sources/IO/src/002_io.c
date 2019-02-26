/*
 * IO example 2: Centralized 2D I/O
 *
 * This example:
 * 1. Read a matrix of integers from a file
 * 2. Block-scatter it among all processes
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
#include <math.h>

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
  char * filename = (argc == 4)?argv[1]:"data/integers.input";

  /* mpi_defs */
  int mpi_rank, mpi_size;
  int grid_rank, grid_coord[2];
  MPI_Comm grid_comm;
  MPI_Datatype contig_t, colblock_t;

  /* other stuff */
  int gsizes[2] = {(argc == 4)?atoi(argv[2]):16, (argc == 4)?atoi(argv[3]):16};
  int lsizes[2],          /* local sizes */
      psizes[2],          /* processes per dim */
      periods[2] = {1,1}; /* periodic grid */
  int *mat = 0, 
      *l_mat = 0;

  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  psizes[1] = (int) floor(sqrt(mpi_size));
  while (mpi_size % psizes[1])
  {
    --psizes[1];
  }
  psizes[0] = mpi_size / psizes[1];

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
    fd = fopen(filename, "r");
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
    MPI_Barrier(MPI_COMM_WORLD);
    if (mpi_rank != p) continue;

    printf("\nProcess {%d,%d}, local size =  %d x %d = %d:\n", 
           grid_coord[0], grid_coord[1], lsizes[0], lsizes[1], lsizes[0] * lsizes[1]);

    /* print */
    for (int y=0; y<lsizes[0]; ++y)
    {
      for (int x=0; x<lsizes[1]; ++x)
      {
        printf("%4d ", l_mat[y*lsizes[1] + x]);
      }
      printf("\n");
    }
    fflush(stdout);
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
    printf("\n");
    for (int y=0; y<gsizes[0]; ++y)
    {
      for (int x=0; x<gsizes[1]; ++x)
        printf("%4d ", mat[y*gsizes[1] + x]);
      printf("\n");
    }

    FILE * fd = fopen(filename, "r");
    if (fwrite(mat, sizeof(int), gsizes[0] * gsizes[1], fd) != gsizes[0]*gsizes[1])
    {
      printf("Error writing output file or dimensions are wrong");
    }
    fclose(fd);
  }

  free(l_mat);
  if (!mpi_rank)
    free(mat);

  MPI_Finalize();

  return 0;
}
