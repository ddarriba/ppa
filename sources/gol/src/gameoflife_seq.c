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

#include "gol.h"

#define LIVE 1

#define MAX_PRINTABLE_WIDTH 80

#define DEFAULT_GENS    80
#define DEFAULT_IFILE   "data/gol.input"
#define DEFAULT_OFILE   "gol.output.bmp"
#define DEFAULT_HEIGHT  16
#define DEFAULT_WIDTH   16

#define EXIT_OK    0
#define ERROR_ARGS 1

#if(LIVE)
#define DISPLAY_SPEED 20000 /* recommended: 200000 */
#endif

#define IOERR 1

void game(state * s, state * s2, int max_gens);

int main(int argc, char **argv)
{
  state s, s2;

  /* input parameters */
  char * filename;
  char * output_filename;
  int gsize[2];
  int max_gens;

  if (argc == 1)
  {
    filename = DEFAULT_IFILE;
    gsize[0] = DEFAULT_HEIGHT;
    gsize[1] = DEFAULT_WIDTH;
    max_gens = DEFAULT_GENS;
    output_filename = DEFAULT_OFILE;
  }
  else if (argc == 5)
  {
    filename = argv[1];
    gsize[0] = atoi(argv[2]);
    gsize[1] = atoi(argv[3]);
    max_gens = atoi(argv[4]);
    output_filename = argc == 6?argv[5]:DEFAULT_OFILE;
  }
  else
  {
    printf("Usage: %s FILENAME ROWS COLS GENS [OUTPUT_BMPFILE]\n", argv[0]);
    return ERROR_ARGS;
  }

  alloc_state(&s, gsize[0], gsize[1]);
  alloc_state(&s2, s.rows, s.cols);

  FILE * ifile = fopen(filename, "r");
  if (!ifile)
  {
    printf("Error %d\n", errno);
    exit(errno);
  }

  for (int y=1; y<=s.rows; ++y)
  {
    int readcnt = fread(s.space[y], sizeof(char), s.cols, ifile);
    if (readcnt != s.cols) {
        fprintf(stderr,
                "ERROR, syntax error in '%s'. fread returned %d instead of %d\n",
                filename, readcnt, s.cols);
        exit(IOERR);
      }
    }
  fclose(ifile);

  game(&s, &s2, max_gens);

  write_bmp_seq(output_filename, &s);

  free_state(&s);
  free_state(&s2);
}

void game(state * s, state * s2, int max_gens)
{

  double sum_gendiff = 0.;
  while (!max_gens || s->generation <= max_gens)
  {
    /* update halo rows */
    memcpy(s->space[0] + 1, s->space[s->rows] + 1, s->cols);
    memcpy(s->space[s->rows+1] + 1, s->space[0] + 1, s->cols);

    /* update halo columns */
    for (int y = 1; y <= s->rows; y++)
    {
      s->space[y][0]  = s->space[y][s->cols];
      s->space[y][s->cols+1] = s->space[y][1];
    }

   /* evolve */
   
#if(LIVE)
    if (s->cols <= MAX_PRINTABLE_WIDTH)
    {
      show(s, 1);
      usleep(DISPLAY_SPEED);
    }
#endif
    sum_gendiff += evolve(s, s2);
  }

#if(!LIVE)
  if (s->cols <= MAX_PRINTABLE_WIDTH)
    show(s, 0);
#endif
}
