/*
 * IO example 3b: MPI I/O using a subarray for non-even distributions
 *
 * Compile: mpicc -Wall -o 04_subarray 04_subarray.c common.c
 *
 * Run: mpirun -np N BIN_NAME Nx Ny
 *      Nx times Ny must equal N
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
      grid_coord[2],        /* process coordinates in grid */
      starts[2];
  MPI_Comm grid_comm;       /* grid communicator */
  MPI_File fh;
  MPI_Datatype filetype_t;

  /* other stuff */
  int *l_mat = 0,
       lsizes[2];

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

  /* local sizes (floor) */
  lsizes[0] = gsizes[0] / psizes[0];
  lsizes[1] = gsizes[1] / psizes[1];

  /* starts, adjusted for remaining rows/cols */
  starts[0] = lsizes[0] * grid_coord[0]
            + ((grid_coord[0] < gsizes[0] % psizes[0])
              ?grid_coord[0]
              :gsizes[0] % psizes[0]);
  starts[1] = lsizes[1] * grid_coord[1]
            + ((grid_coord[1] < gsizes[1] % psizes[1])
              ?grid_coord[1]
              :gsizes[1] % psizes[1]);

  /* adjust sizes for remaining rows/cols */
  if (grid_coord[0] < gsizes[0] % psizes[0])
    ++lsizes[0];
  if (grid_coord[1] < gsizes[1] % psizes[1])
    ++lsizes[1];







  /* 1. Read the input file */
  l_mat = (int *) malloc( lsizes[0] * lsizes[1] * sizeof(int) );
  MPI_File_open(MPI_COMM_WORLD, input_filename,
                MPI_MODE_RDONLY,
                MPI_INFO_NULL, &fh);

  MPI_Type_create_subarray(2,
                           gsizes,
                           lsizes,
                           starts,
                           MPI_ORDER_C,
                           MPI_INT,
                           &filetype_t);
  MPI_Type_commit(&filetype_t);

  MPI_File_set_view(fh, 0, MPI_INT, filetype_t, "native", MPI_INFO_NULL);

  MPI_File_read(fh, l_mat, lsizes[0] * lsizes[1], MPI_INT, MPI_STATUS_IGNORE);
  MPI_File_close(&fh);


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

  /* 2. Apply function f to every element */
  for (int y=0; y<lsizes[0]; ++y)
    for (int x=0; x<lsizes[1]; ++x)
      l_mat[y*lsizes[1] + x] = f(l_mat[y*lsizes[1] + x]);






  /* 3. Print matrix to an output file */
  MPI_File_open(MPI_COMM_WORLD, output_filename,
                MPI_MODE_CREATE | MPI_MODE_WRONLY,
                MPI_INFO_NULL, &fh);

  MPI_File_set_view(fh, 0, MPI_INT, filetype_t, "native", MPI_INFO_NULL);

  MPI_File_write(fh, l_mat, lsizes[0] * lsizes[1], MPI_INT, MPI_STATUS_IGNORE);
  MPI_File_close(&fh);






  free(l_mat);



  MPI_Finalize();

  return 0;
}
