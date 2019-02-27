/*
 * MPI IO example 1
 *
 * Old times file reading and scattering
 */
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <mpi.h>
#include <math.h>

#include "gol.h"

#define UP    0
#define DOWN  1
#define LEFT  2
#define RIGHT 3

#define ROWS 0
#define COLS 1

#define DEFAULT_GENS 10

#define DEBUG_START_MATRIX   0
#define DEBUG_LOCAL_MATRICES 0
#define DEBUG_FINAL_MATRICES 0
#define DEBUG_FINAL_MATRIX   0

typedef struct {
  int rank;
  int size;
  int neighbor[4];
  int dim[2];
  int coord[2];
  MPI_Comm comm;
} parallel_state;

double game(state * s, state * s2, int max_gens, parallel_state * mpi);

MPI_Datatype mpi_lcontig_t, mpi_lrow_t, mpi_lcol_t,
               mpi_scontig_t,  mpi_sblock_t,
               mpi_filetype_t;

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  /* input parameters */
  char * filename = (argc >= 4)?argv[1]:"data/gol.input";
  int gsize[2] = {(argc >= 4)?atoi(argv[2]):16, (argc >= 4)?atoi(argv[3]):16};
  int lsize[2];
  int max_gens;

  max_gens = (argc == 2)?atoi(argv[1]):(argc == 5)?atoi(argv[4]):DEFAULT_GENS;

  /* mpi_defs */
  parallel_state mpi;
  int warp_around[2] = {1,1};
  MPI_File mpi_file;
  MPI_Status status;

  /* other stuff */
  state s, s2;
  char *mat = 0;
  long checksum;

  MPI_Comm_rank(MPI_COMM_WORLD, &mpi.rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi.size);

  mpi.dim[1] = (int) floor(sqrt(mpi.size));
  while (mpi.size % mpi.dim[1])
  {
    --mpi.dim[1];
  }
  mpi.dim[0] = mpi.size / mpi.dim[1];

  /* 2D cartesian topology */
  MPI_Cart_create(MPI_COMM_WORLD, 2, mpi.dim, warp_around, 0, &mpi.comm);
  MPI_Cart_coords(mpi.comm, mpi.rank, 2, mpi.coord);
  MPI_Cart_shift(mpi.comm, 1, 1,
		 &mpi.neighbor[LEFT], &mpi.neighbor[RIGHT]);
  MPI_Cart_shift(mpi.comm, 0, 1,
                 &mpi.neighbor[UP], &mpi.neighbor[DOWN]);

  lsize[ROWS] = gsize[ROWS]/mpi.dim[ROWS];
  lsize[COLS] = gsize[COLS]/mpi.dim[COLS];

  /* initialize state */
  alloc_state(&s, lsize[ROWS], lsize[COLS]);
  alloc_state(&s2, lsize[ROWS], lsize[COLS]);

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
  MPI_Type_create_resized(mpi_scontig_t,     /* input datatype */
                          0,            /* new lower bound */
                          sizeof(char),  /* new extent */
                          &mpi_sblock_t); /* new datatype (output) */
  MPI_Type_commit(&mpi_sblock_t);

  int distribs[2] = {MPI_DISTRIBUTE_BLOCK, MPI_DISTRIBUTE_BLOCK};
  int dargs[2] = {MPI_DISTRIBUTE_DFLT_DARG, MPI_DISTRIBUTE_DFLT_DARG};
  MPI_Type_create_darray(mpi.size,
                         mpi.rank,
                         2,
                         gsize,
                         distribs,
                         dargs,
                         mpi.dim,
                         MPI_ORDER_C, /* C style, row-major order */
                         MPI_CHAR,
                         &mpi_filetype_t);
  MPI_Type_commit(&mpi_filetype_t);

  for (int p=0; p<mpi.size; ++p)
  {
    if (mpi.rank == p)
    {
      printf("\nProcess %d/%d (%d,%d) of (%d,%d), local size =  %d x %d = %d:\n", 
             mpi.rank, mpi.size,
             mpi.coord[ROWS], mpi.coord[COLS], 
             mpi.dim[ROWS], mpi.dim[COLS],
             s.rows, s.cols, s.rows * s.cols);
      printf("  Neighbors UP: %d DOWN: %d LEFT: %d RIGHT: %d\n", 
             mpi.neighbor[UP], mpi.neighbor[DOWN], mpi.neighbor[LEFT], mpi.neighbor[RIGHT]);
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }

  /* read file with MPI I/O */
  MPI_File_open(mpi.comm, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &mpi_file);
  MPI_File_set_view(mpi_file, 0, MPI_CHAR, mpi_filetype_t, "native", MPI_INFO_NULL);
  MPI_File_read(mpi_file, &s.space[1][1], s.rows, mpi_lrow_t, &status);
  MPI_File_close(&mpi_file);

#if(DEBUG_LOCAL_MATRICES)
  /* show individual boards */
  for (int p=0; p<mpi.size; ++p)
  {
    if (mpi.rank == p)
    {
      printf("\nProcess %d/%d (%d,%d) of (%d,%d), local size =  %d x %d = %d:\n", 
             mpi.rank, mpi.size,
             mpi.coord[ROWS], mpi.coord[COLS], 
             mpi.dim[ROWS], mpi.dim[COLS],
             s.rows, s.cols, s.rows * s.cols);
      printf("  Neighbors UP: %d DOWN: %d LEFT: %d RIGHT: %d\n", 
             mpi.neighbor[UP], mpi.neighbor[DOWN], mpi.neighbor[LEFT], mpi.neighbor[RIGHT]);
      show(&s,0);
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }
#endif

  checksum = game(&s, &s2, max_gens, &mpi);

  MPI_Reduce(mpi.rank?&checksum:MPI_IN_PLACE, &checksum, 1,
               MPI_DOUBLE, MPI_SUM, 0,
               MPI_COMM_WORLD);

#if(DEBUG_FINAL_MATRICES)
  /* show individual boards */
  for (int p=0; p<mpi.size; ++p)
  {
    if (mpi.rank == p)
    {
      printf("\nProcess %d/%d, local size =  %d x %d = %d:\n", 
             mpi.rank, mpi.size, s.rows, s.cols, s.rows * s.cols);
      printf("  Neighbors UP: %d DOWN: %d \n", 
             mpi.neighbor[UP], mpi.neighbor[DOWN]);
      show(&s,0);
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }
#endif

  write_bmp("gol_output.bmp", &s, gsize, mpi.dim, mpi.comm);

  /* gather matrix back to root */
  int disps[mpi.size];
  int counts[mpi.size];
  for (int y=0; y<mpi.dim[0]; y++) {
      for (int x=0; x<mpi.dim[1]; x++) {
          disps[y*mpi.dim[1]+x] = y*gsize[1]*lsize[0]+x*lsize[1];
          counts [y*mpi.dim[1]+x] = 1;
      }
  }

#if(DEBUG_FINAL_MATRIX)
  if (!mpi.rank)
    mat = (char *) malloc(gsize[ROWS] * gsize[COLS] * sizeof(char));

  MPI_Gatherv(&s.space[1][1], s.rows, mpi_lrow_t,
              mat, counts, disps, mpi_sblock_t,
              0, mpi.comm);
#endif

  if (!mpi.rank)
  {
    /* print final board */
    printf("\n\n\n\nChecksum: %ld\n", checksum);
#if(DEBUG_FINAL_MATRIX)
    show_space(mat,gsize[ROWS],gsize[COLS],0,0);
#endif
  }

  free_state(&s);
  free_state(&s2);
  if (!mpi.rank)
    free(mat);

  MPI_Finalize();

  return 0;
}




double game(state * s, state * s2, int max_gens, parallel_state * mpi)
{
  double sum_gendiff = 0.;
  for (int gen=0; gen < max_gens; ++gen)
  {

    /* Communicate in y-direction */  
    MPI_Sendrecv(s->space[s->rows]+1, s->cols, MPI_CHAR, mpi->neighbor[DOWN], UP,
                 s->space[0]+1, s->cols, MPI_CHAR, mpi->neighbor[UP], UP,
                 mpi->comm, MPI_STATUS_IGNORE);
    MPI_Sendrecv(s->space[1]+1, s->cols, MPI_CHAR, mpi->neighbor[UP], DOWN,
                 s->space[s->rows+1]+1, s->cols, MPI_CHAR, mpi->neighbor[DOWN], DOWN,
                 mpi->comm, MPI_STATUS_IGNORE);

    /* Communicate in x-direction */
    MPI_Sendrecv(s->space[1]+s->cols, 1, mpi_lcol_t, mpi->neighbor[RIGHT], RIGHT,
                 s->space[1],           1, mpi_lcol_t, mpi->neighbor[LEFT], RIGHT,
                 mpi->comm, MPI_STATUS_IGNORE);
    MPI_Sendrecv(s->space[1]+1,         1, mpi_lcol_t, mpi->neighbor[LEFT], LEFT,
                 s->space[1]+s->cols+1, 1, mpi_lcol_t, mpi->neighbor[RIGHT], LEFT,
                 mpi->comm, MPI_STATUS_IGNORE);

    /* Swap corner data in auxiliary rows*/
    char scornerbuf[4] = {s->space[0][s->cols], s->space[s->rows+1][s->cols], s->space[0][1], s->space[s->rows+1][1]};
    char rcornerbuf[4];
    MPI_Sendrecv(scornerbuf, 2, MPI_CHAR, mpi->neighbor[RIGHT], RIGHT,
                 rcornerbuf, 2, MPI_CHAR, mpi->neighbor[LEFT], RIGHT,
                 mpi->comm, MPI_STATUS_IGNORE);
    MPI_Sendrecv(scornerbuf+2, 2, MPI_CHAR, mpi->neighbor[LEFT], LEFT,
                 rcornerbuf+2, 2, MPI_CHAR, mpi->neighbor[RIGHT], LEFT,
                 mpi->comm, MPI_STATUS_IGNORE);

    s->space[0][0] = rcornerbuf[0];
    s->space[s->rows+1][0] = rcornerbuf[1];
    s->space[0][s->cols+1] = rcornerbuf[2];
    s->space[s->rows+1][s->cols+1] = rcornerbuf[3];

#if(DEBUG_LOCAL_MATRICES)
    /* print local extended matrices */
    for (int i=0; i<mpi->size; i++)
    {
      MPI_Barrier(MPI_COMM_WORLD);
      if (mpi->rank != i) continue;

      printf("RANK %d (%d,%d)\n", mpi->rank, mpi->coord[ROWS], mpi->coord[COLS]);
      for (int y=0; y<=s->rows+1; ++y)
      {
        for (int x=0; x<=s->cols+1; ++x)
        {
          printf("%2d ", s->space[y][x]);
          if (!x || x == s->cols)
            printf(" ");
        }
        printf("\n");
        if (!y || y == s->rows)
           printf("\n");
      }
      printf("\n\n");
    }
#endif

    sum_gendiff += evolve(s, s2);
  }

  return sum_gendiff;
}
