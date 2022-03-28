/*
 * Game of Life (sequential)
 *
 * code derived from rosettacode.org
 *
 * Usage:
 *  ./gameoflife_seq FILENAME ROWS COLS GENS [OUTPUT_BMPFILE]
 *
 *  FILENAME is the input binary file containing the starting state
 *  ROWS/COLS is the size of the starting state
 *  GENS is the number of generations (0 for no limit)
 *  OUTPUT_BMPFILE is the picture of the final state
 *
 * Run with default test matrix:
 *  ./gameoflife_seq
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <mpi.h>
#include <math.h>

#include "gol_common.h"

#define WITH_HALO 1

#define UP    0
#define DOWN  1
#define LEFT  2
#define RIGHT 3

#define EXIT_OK    0
#define ERROR_ARGS 1
#define ERROR_PDIM 2

#define IOERR 1

typedef struct {
  int rank;        /* mpi rank */
  int size;        /* mpi size */
  int neighbor[4]; /* mpi neighbor ranks */
  int dim[2];      /* mpi proc grid dimensions */
  int coord[2];    /* mpi proc grid coordinate */
  MPI_Comm comm;   /* mpi intercommunicator */
} parallel_state;

void game(state * s, int max_gens, parallel_state * mpi);
void swap_halo(state * s, parallel_state * mpi);
int read_input(state * s, const char * filename, const int *gsize, parallel_state * mpi);
void print_state(state * s, const char * filename, int *gsizes, parallel_state * mpi);

MPI_Datatype mpi_lcontig_t, mpi_lrow_t, mpi_lcol_t,
             mpi_scontig_t,  mpi_sblock_t;

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  state s;

  /* input parameters */
  char * filename;
  char * output_filename;
  int gsize[2];
  int max_gens;

  /* MPI */
  parallel_state mpi;
  int lsize[2],
      warp_around[2] = {1,1}; /* cyclic game space? {vertical, horizontal} */

  /* runtimes */
  double s_time, i_time, c0_time, c1_time, e_time;

  if (!parse_arguments(argc, argv, &filename, gsize, &max_gens, &output_filename))
  {
    printf("Usage: %s FILENAME ROWS COLS GENS [OUTPUT_BMPFILE]\n", argv[0]);
    return ERROR_ARGS;
  }

  MPI_Comm_rank(MPI_COMM_WORLD, &mpi.rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi.size);

  /* auto-set 2D processors grid */
  mpi.dim[COLS] = (int) floor(sqrt(mpi.size));
  while (mpi.size % mpi.dim[COLS])
  {
    --mpi.dim[COLS];
  }
  mpi.dim[ROWS] = mpi.size / mpi.dim[COLS];

  if ((gsize[ROWS] % mpi.dim[ROWS] > 0) || (gsize[COLS] % mpi.dim[COLS] > 0))
  {
    if (mpi.rank == 0)
    {
      printf("Error: Matrix size must be divisible by the number of processes on each dimension.\n");
      printf("       Dim 0: %d rows, %d processes\n", gsize[ROWS], mpi.dim[ROWS]);
      printf("       Dim 1: %d columns, %d processes\n", gsize[COLS], mpi.dim[COLS]);
    }
    MPI_Finalize();
    return ERROR_PDIM;
  }
  else if (mpi.rank == 0)
  {
      printf("Setting up a %d by %d processes grid\n\n", mpi.dim[ROWS], mpi.dim[COLS]);
  }

  /* set 2D cartesian topology */
  MPI_Cart_create(MPI_COMM_WORLD, 2, mpi.dim, warp_around, 0, &mpi.comm);
  MPI_Cart_coords(mpi.comm, mpi.rank, 2, mpi.coord);
  MPI_Cart_shift(mpi.comm, 1, 1,
     &mpi.neighbor[LEFT], &mpi.neighbor[RIGHT]);
  MPI_Cart_shift(mpi.comm, 0, 1,
                 &mpi.neighbor[UP], &mpi.neighbor[DOWN]);

  /* calculate local sizes */
  lsize[ROWS] = gsize[ROWS]/mpi.dim[ROWS];
  lsize[COLS] = gsize[COLS]/mpi.dim[COLS];

  alloc_state(&s, lsize[ROWS], lsize[COLS], WITH_HALO);

  /* create extended block datatypes */
  MPI_Type_contiguous(s.cols, MPI_CHAR, &mpi_lcontig_t);
  MPI_Type_create_resized(mpi_lcontig_t, /* input datatype */
                          0,             /* new lower bound */
                          s.cols+2,      /* new extent */
                          &mpi_lrow_t);  /* new datatype (output) */
  MPI_Type_commit(&mpi_lrow_t);

  MPI_Type_vector(s.rows, 1, s.cols+2, MPI_CHAR, &mpi_lcol_t);
  MPI_Type_commit(&mpi_lcol_t);

  MPI_Type_vector(lsize[ROWS], lsize[COLS], gsize[COLS], MPI_CHAR, &mpi_scontig_t);
  MPI_Type_create_resized(mpi_scontig_t,  /* input datatype */
                          0,              /* new lower bound */
                          sizeof(char),   /* new extent */
                          &mpi_sblock_t); /* new datatype (output) */
  MPI_Type_commit(&mpi_sblock_t);

  /* print grid configuration */
  for (int p=0; p<mpi.size; ++p)
  {
    if (mpi.rank == p)
    {
      printf("Process %d/%d (%d,%d) of (%d,%d), local size =  %d x %d = %d:\n",
             mpi.rank, mpi.size,
             mpi.coord[ROWS], mpi.coord[COLS],
             mpi.dim[ROWS], mpi.dim[COLS],
             s.rows, s.cols, s.rows * s.cols);
      printf("  Neighbors UP: %d DOWN: %d LEFT: %d RIGHT: %d\n\n",
             mpi.neighbor[UP], mpi.neighbor[DOWN],
             mpi.neighbor[LEFT], mpi.neighbor[RIGHT]);
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }

  s_time = MPI_Wtime();

  /* read the initial state from file */
  if (read_input(&s, filename, gsize, &mpi) != MPI_SUCCESS)
    MPI_Abort(MPI_COMM_WORLD, IOERR);

  i_time = MPI_Wtime();

  game(&s, max_gens, &mpi);

  c0_time = MPI_Wtime();

  for (int p=0; p<mpi.size; ++p)
  {
    if (mpi.rank == p)
    {
      printf("Process (%d,%d): Local Checksum %ld\n",
             mpi.coord[ROWS], mpi.coord[COLS], s.checksum);
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }

  //TODO: Replace the Reduction with an Accumulate operation
  MPI_Reduce(mpi.rank?&s.checksum:MPI_IN_PLACE, &s.checksum, 1,
               MPI_LONG, MPI_SUM, 0,
               MPI_COMM_WORLD);

  if (!mpi.rank)
    printf("\nGlobal Checksum after %ld generations: %ld\n", s.generation, s.checksum);

  c1_time = MPI_Wtime();

  /* draw the final space state in a bmp image */
  write_bmp_mpi(output_filename, &s, gsize, mpi.dim, mpi.comm);
  if (!mpi.rank)
    printf("\nFinal state dumped to %s\n", output_filename);

  /* dump the final space state */
  print_state(&s, "output", gsize, &mpi);

  e_time = MPI_Wtime();

  if (!mpi.rank)
  {
    printf("\nRuntimes:\n");
    printf("  Input: %lf seconds\n", i_time - s_time);
    printf("  Computation: %lf seconds\n", c0_time - i_time);
    printf("  Output: %lf seconds\n", e_time - c1_time);
  }
  free_state(&s);

  MPI_Finalize();
}

void game(state * s, int max_gens, parallel_state * mpi)
{
  long sum_gendiff = 0.;

  //show(s, 0); /* This line prints to stdout the inital state */
  while (s->generation < max_gens)
  {
    assert(s->halo);

    swap_halo(s, mpi);

    /* evolve */
    sum_gendiff += evolve(s);
  }
}

/*
 * Place bounding rows, columns and corners into adjacent halos
 */
void swap_halo(state * s, parallel_state * mpi)
{
  //TODO: Replace this function body with MPI RMA
  
  MPI_Request req0, req1;
  
  /* Communicate in y-direction */

  MPI_Isend(s->space[s->rows]+1, s->cols, MPI_CHAR, mpi->neighbor[DOWN], UP, mpi->comm, &req0);
  MPI_Recv(s->space[0]+1, s->cols, MPI_CHAR, mpi->neighbor[UP], UP, mpi->comm, MPI_STATUS_IGNORE);
  MPI_Isend(s->space[1]+1, s->cols, MPI_CHAR, mpi->neighbor[UP], DOWN, mpi->comm, &req0);
  MPI_Recv(s->space[s->rows+1]+1, s->cols, MPI_CHAR, mpi->neighbor[DOWN], DOWN, mpi->comm, MPI_STATUS_IGNORE);

  /* Communicate in x-direction */

  MPI_Isend(s->space[1]+s->cols, 1, mpi_lcol_t, mpi->neighbor[RIGHT], RIGHT, mpi->comm, &req0);
  MPI_Recv(s->space[1],           1, mpi_lcol_t, mpi->neighbor[LEFT], RIGHT, mpi->comm, MPI_STATUS_IGNORE);
  MPI_Isend(s->space[1]+1,         1, mpi_lcol_t, mpi->neighbor[LEFT], LEFT, mpi->comm, &req1);
  MPI_Recv(s->space[1]+s->cols+1, 1, mpi_lcol_t, mpi->neighbor[RIGHT], LEFT, mpi->comm, MPI_STATUS_IGNORE);

  /* Swap corner data in auxiliary rows*/
  char scornerbuf[4] = {s->space[0][s->cols], s->space[s->rows+1][s->cols], s->space[0][1], s->space[s->rows+1][1]};
  char rcornerbuf[4];

  MPI_Isend(scornerbuf, 2, MPI_CHAR, mpi->neighbor[RIGHT], RIGHT, mpi->comm, &req0);
  MPI_Recv(rcornerbuf, 2, MPI_CHAR, mpi->neighbor[LEFT], RIGHT, mpi->comm, MPI_STATUS_IGNORE);
  MPI_Isend(scornerbuf+2, 2, MPI_CHAR, mpi->neighbor[LEFT], LEFT, mpi->comm, &req0);
  MPI_Recv(rcornerbuf+2, 2, MPI_CHAR, mpi->neighbor[RIGHT], LEFT, mpi->comm, MPI_STATUS_IGNORE);

  s->space[0][0] = rcornerbuf[0];
  s->space[s->rows+1][0] = rcornerbuf[1];
  s->space[0][s->cols+1] = rcornerbuf[2];
  s->space[s->rows+1][s->cols+1] = rcornerbuf[3];
}

/*
 * Reads the input file and fills the initial state space
 *
 * Returns MPI_SUCCESS if the read was correct or an error code otherwise.
 */
int read_input(state * s, const char * filename, const int *gsize, parallel_state * mpi)
{
  //TODO: Replace this function body with RMA I/O
  
  char *mat = 0;
  int disps[mpi->size];
  int counts[mpi->size];
  int return_val;

  if (!mpi->rank)
  {
    /* read initial state from file */
    mat = (char *) malloc(gsize[ROWS] * gsize[COLS] * sizeof(char));

    FILE * ifile = fopen(filename, "r");
    if (!ifile)
    {
      fprintf(stderr, "Error: %s %s\n", strerror(errno), filename);
      fflush(stderr);
      return errno;
    }

    for (int y=0; y<gsize[ROWS]; ++y)
    {
      int readcnt = fread(mat + y * gsize[COLS], sizeof(char), gsize[COLS], ifile);
      if (readcnt != gsize[COLS]) {
          fprintf(stderr,
                  "ERROR, syntax error in '%s'. fread returned %d instead of %d\n",
                  filename, readcnt, gsize[COLS]);
          fprintf(stderr,
                  "       check if size (%d, %d) is correct for '%s'\n",
                  s->rows, s->cols, filename);
          fflush(stderr);
          return errno;
      }
    }
    fclose(ifile);
  }

  /* scatter matrix among all processes */
  for (int y=0; y<mpi->dim[ROWS]; y++) {
      for (int x=0; x<mpi->dim[COLS]; x++) {
          disps[y*mpi->dim[COLS]+x] = y*gsize[COLS]*s->rows+x*s->cols;
          counts [y*mpi->dim[COLS]+x] = 1;
      }
  }

  return_val = MPI_Scatterv(mat, counts, disps, mpi_sblock_t,
                            &s->space[1][1], s->rows, mpi_lrow_t,
                            0, mpi->comm);

  if (mat) free(mat);

  if (return_val != MPI_SUCCESS)
  {
    char err_string[MPI_MAX_ERROR_STRING];
    int len;
    MPI_Error_string(return_val, err_string, &len);
    fprintf(stderr,"Error %d: %s\n", return_val, err_string);
    fflush(stderr);
  }
  return return_val;
}

void print_state(state * s, const char * filename, int *gsize, parallel_state * mpi)
{
  // TODO: Replace this function body with MPI I/O
  
  // This function should write in parallel the space of state "s" to a file
  // The output file must be also a valid input file
  // If GoL is run with '0' iterations, the output file should be equal to input file
  
  char *mat = 0;
  int disps[mpi->size];
  int counts[mpi->size];
  int return_val;

  assert(s->halo);
  
  if (!mpi->rank)
  {
    /* read initial state from file */
    mat = (char *) malloc(gsize[ROWS] * gsize[COLS] * sizeof(char));
  }

  /* gather matrix from all processes */
  for (int y=0; y<mpi->dim[ROWS]; y++) {
      for (int x=0; x<mpi->dim[COLS]; x++) {
          disps[y*mpi->dim[COLS]+x] = y*gsize[COLS]*s->rows+x*s->cols;
          counts [y*mpi->dim[COLS]+x] = 1;
      }
  }

  return_val = MPI_Gatherv(&s->space[1][1], s->rows, mpi_lrow_t,
                           mat, counts, disps, mpi_sblock_t,
                           0, mpi->comm);

  if (return_val != MPI_SUCCESS)
  {
    char err_string[MPI_MAX_ERROR_STRING];
    int len;
    MPI_Error_string(return_val, err_string, &len);
    fprintf(stderr,"Error %d: %s\n", return_val, err_string);
    fflush(stderr);
    MPI_Finalize();
    exit(1);
  }
  
  if (mpi->rank == 0)
  {
    FILE * ofile = fopen(filename, "w");
  
    fwrite(mat, sizeof(char), gsize[ROWS]*gsize[COLS], ofile);

    fclose(ofile);
    free(mat);
  }  
}
