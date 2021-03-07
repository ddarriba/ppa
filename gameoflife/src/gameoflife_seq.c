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

#include "gol_common.h"

#ifndef LIVE
  #define LIVE 0
#endif

#define WITH_HALO 1

#define EXIT_OK    0
#define ERROR_ARGS 1

#if(LIVE)
#define DISPLAY_DELAY 20000 /* recommended: 200000 */
#endif

#define IOERR 1

void game(state * s, int max_gens);

int main(int argc, char **argv)
{
  state s;

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
    max_gens = LIVE?0:DEFAULT_GENS;
    output_filename = DEFAULT_OFILE;

    printf("Run default configuration:\n");
    printf("  Input file: %s\n", filename);
    printf("  Height:     %d\n", gsize[0]);
    printf("  Width:      %d\n", gsize[1]);
    printf("  Gens:       %d\n", max_gens);
  }
  else if (argc >= 5)
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

  alloc_state(&s, gsize[0], gsize[1], WITH_HALO);

  FILE * ifile = fopen(filename, "r");
  if (!ifile)
  {
    printf("Error %d\n", errno);
    exit(errno);
  }

  for (int y=s.halo; y<s.rows+s.halo; ++y)
  {
    int readcnt = fread(s.space[y]+s.halo, sizeof(char), s.cols, ifile);
    if (readcnt != s.cols) {
        fprintf(stderr,
                "ERROR, syntax error in '%s'. fread returned %d instead of %d\n",
                filename, readcnt, s.cols);
        exit(IOERR);
    }
  }
  fclose(ifile);

  game(&s, max_gens);
  printf("\nGlobal Checksum after %ld generations: %ld\n", s.generation, s.checksum);

  write_bmp(output_filename, &s);
  printf("\nFinal state dumped to %s\n", output_filename);

  free_state(&s);
}

void game(state * s, int max_gens)
{
  double sum_gendiff = 0.;
  while ((!max_gens && LIVE) || s->generation < max_gens)
  {
    if (s->halo)
    {
      /* update halo rows */
      memcpy(s->space[0] + 1, s->space[s->rows] + 1, s->cols);
      memcpy(s->space[s->rows+1] + 1, s->space[1] + 1, s->cols);

      /* update halo columns */
      for (int y = 1; y <= s->rows; y++)
      {
        s->space[y][0]  = s->space[y][s->cols];
        s->space[y][s->cols+1] = s->space[y][1];
      }

      /* update halo corners */
      s->space[0][0] = s->space[s->rows][s->cols];
      s->space[0][s->cols+1] = s->space[s->rows][1];
      s->space[s->rows+1][0] = s->space[1][s->cols];
      s->space[s->rows+1][s->cols+1] = s->space[1][1];
    }

    /* evolve */

#if(LIVE)
    show(s, LIVE);
    usleep(DISPLAY_DELAY);
#endif
    sum_gendiff += evolve(s);
  }

  //show(s, LIVE);  /* This line prints to stdout the final state */
}
