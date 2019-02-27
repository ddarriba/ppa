/*
 * IO example 5: MPI I/O using a distributed array
 *
 * Run: mpirun -np N bin/005_io Nx Ny
 *      Nx times Ny must equal N
 */
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <mpi.h>

#define DEFAULT_INPUT_FN  "data/integers.input"

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
  char * input_filename  = DEFAULT_INPUT_FN;
           
  int h = 16;
  int w = 16;

  /* mpi_defs */
  int mpi_rank, mpi_size, mpi_size_x, mpi_size_y;
  MPI_File fh;
  MPI_Datatype filetype_t;

  /* other stuff */
  int *l_mat = 0,
       l_h,
       l_w;

  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  if (argc != 3)
  {
    if (!mpi_rank)
      printf("Usage: mpirun -np N %s Nx Ny\n", argv[0]);
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  mpi_size_x = atoi(argv[1]);
  mpi_size_y = atoi(argv[2]);

  if (mpi_size_x * mpi_size_y != mpi_size)
  {
    if (!mpi_rank)
      printf("Nx (%d) times Ny (%d) must equal N (%d)\n",
             mpi_size_x, mpi_size_y, mpi_size);
    MPI_Abort(MPI_COMM_WORLD, 2);
  }

  l_h = h/mpi_size_y;
  l_w = w/mpi_size_x;

  /* 1. Read the input file */
  l_mat = (int *) malloc( l_h * l_w * sizeof(int) );
  MPI_File_open(MPI_COMM_WORLD, input_filename,
                MPI_MODE_RDONLY,
                MPI_INFO_NULL, &fh);

  /* play here with BLOCK / CYCLIC distributions */
  /* for CYCLIC distributions, different dargs */
  int distribs[2] = {MPI_DISTRIBUTE_BLOCK, MPI_DISTRIBUTE_BLOCK};
  int dargs[2] = {MPI_DISTRIBUTE_DFLT_DARG, MPI_DISTRIBUTE_DFLT_DARG};
  int gsize[2] = {h, w};
  int mpi_dim[2] = {mpi_size_y, mpi_size_x};

  MPI_Type_create_darray(mpi_size,
                         mpi_rank,
                         2,           /* number of dimensions */
                         gsize,       /* global size */
                         distribs,    /* block or cyclic */
                         dargs,       /* distribution size */
                         mpi_dim,     /* mpi dimensions */
                         MPI_ORDER_C, /* C style, row-major order */
                         MPI_INT,     /* etype */
                         &filetype_t);
  MPI_Type_commit(&filetype_t);

  MPI_File_set_view(fh, 0, MPI_INT, filetype_t, "native", MPI_INFO_NULL);

  MPI_File_read(fh, l_mat, l_h * l_w, MPI_INT, MPI_STATUS_IGNORE);
  MPI_File_close(&fh);

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

  free(l_mat);

  MPI_Finalize();

  return 0;
}
