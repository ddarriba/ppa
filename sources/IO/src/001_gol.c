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

#include "gol.h"

#define UP   0
#define DOWN 1

#define ROWS 0
#define COLS 1

#define DEFAULT_GENS 10

#define DEBUG_START_MATRIX   0
#define DEBUG_LOCAL_MATRICES 0
#define DEBUG_FINAL_MATRICES 0

typedef struct {
  int rank;
  int size;
  int neighbor[2];
  MPI_Comm comm;
} parallel_state;

double game(state * s, state * s2, int max_gens, parallel_state * mpi);

MPI_Datatype mpi_lcontig_t, mpi_lrow_t,
             mpi_contig_t,  mpi_row_t;

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  /* input parameters */
  char * filename = (argc >= 4)?argv[1]:"gol.input";
  int gsize[2] = {(argc >= 4)?atoi(argv[2]):16, (argc >= 4)?atoi(argv[3]):16};
  int lsize[2];
  int max_gens;

  max_gens = (argc == 2)?atoi(argv[1]):(argc == 5)?atoi(argv[4]):DEFAULT_GENS;

  /* mpi_defs */
  parallel_state mpi;

  /* other stuff */
  state s, s2;
  char *mat = 0;
  long checksum;

  MPI_Comm_rank(MPI_COMM_WORLD, &mpi.rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi.size);
  mpi.neighbor[UP] = (mpi.rank + mpi.size - 1) % mpi.size;
  mpi.neighbor[DOWN] = (mpi.rank + 1) % mpi.size;
  mpi.comm = MPI_COMM_WORLD;

  lsize[ROWS] = gsize[ROWS]/mpi.size;
  lsize[COLS] = gsize[COLS];

  /* initialize state */
  alloc_state(&s, lsize[ROWS], lsize[COLS]);
  alloc_state(&s2, lsize[ROWS], lsize[COLS]);

  /* create extended row datatype */
  MPI_Type_contiguous(s.cols, MPI_CHAR, &mpi_lcontig_t);
  MPI_Type_create_resized(mpi_lcontig_t, /* input datatype */
                          0,             /* new lower bound */
                          s.cols+2,      /* new extent */
                          &mpi_lrow_t);  /* new datatype (output) */
  MPI_Type_commit(&mpi_lrow_t);

  if (!mpi.rank)
  {
    /* read initial state from file */
    mat = (char *) malloc(gsize[ROWS] * gsize[COLS] * sizeof(char));

    FILE * fd;
    fd = fopen(filename, "r");
    if (fread(mat, sizeof(char), gsize[ROWS]*gsize[COLS], fd) != gsize[ROWS]*gsize[COLS])
    {
      printf("Error reading input file or dimensions are wrong");
    }
    fclose(fd);

#if(DEBUG_START_MATRIX)
    show_space(mat,gsize[ROWS],gsize[COLS],0,0);
#endif
  }

  /* scatter matrix among all processes */
  MPI_Scatter(mat, s.rows * s.cols, MPI_CHAR,
              &s.space[1][1], s.rows, mpi_lrow_t,
              0, MPI_COMM_WORLD);

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

  /* gather matrix back to root */
  MPI_Gather(&s.space[1][1], s.rows, mpi_lrow_t,
             mat, s.rows * s.cols, MPI_CHAR,
             0, MPI_COMM_WORLD);

  if (!mpi.rank)
  {
    /* print final board */
    printf("\n\n\n\nChecksum: %ld\n", checksum);
    show_space(mat,gsize[ROWS],gsize[COLS],0,0);
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

    /* place first/last columns */
    for (int y = 0; y <= s->rows+1; y++) 
    {
      s->space[y][0]  = s->space[y][s->cols];
      s->space[y][s->cols+1] = s->space[y][1];
    }

#if(DEBUG_LOCAL_MATRICES)
    /* print local extended matrices */
    for (int i=0; i<mpi->size; i++)
    {
      MPI_Barrier(MPI_COMM_WORLD);
      if (mpi->rank != i) continue;
      printf("RANK %d\n", mpi->rank);
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
