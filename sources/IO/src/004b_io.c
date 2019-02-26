/*
 * IO example 4b: MPI I/O using a non-contiguous File View
 *
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
  char * input_filename  = (argc == 4)?argv[1]:DEFAULT_INPUT_FN;
           
  int h = (argc == 4)?atoi(argv[2]):16;
  int w = (argc == 4)?atoi(argv[3]):16;

  /* mpi_defs */
  int mpi_rank, mpi_size;
  MPI_File fh;
  MPI_Datatype contig_t, filetype_t;

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

  MPI_File_read(fh, l_mat, l_h * l_w, MPI_INT, MPI_STATUS_IGNORE);
  MPI_File_close(&fh);

  /* Print what each process have */

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
