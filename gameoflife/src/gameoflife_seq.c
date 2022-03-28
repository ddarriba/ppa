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
void swap_halo(state * s);
void print_state(state * s, const char * filename, int *gsize);

int main(int argc, char **argv)
{
  state s;

  /* input parameters */
  char * filename;
  char * output_filename;
  int gsize[2];
  int max_gens;

  if (!parse_arguments(argc, argv, &filename, gsize, &max_gens, &output_filename))
  {
    printf("Usage: %s FILENAME ROWS COLS GENS [OUTPUT_BMPFILE]\n", argv[0]);
    return ERROR_ARGS;
  }

  alloc_state(&s, gsize[ROWS], gsize[COLS], WITH_HALO);

  FILE * ifile = fopen(filename, "r");
  if (!ifile)
  {
    fprintf(stderr, "Error: %s %s\n", strerror(errno), filename);
    exit(errno);
  }

  for (int y=s.halo; y<s.rows+s.halo; ++y)
  {
    int readcnt = fread(s.space[y]+s.halo, sizeof(char), s.cols, ifile);
    if (readcnt != s.cols) {
        fprintf(stderr,
                "ERROR, syntax error in '%s'. fread returned %d instead of %d\n",
                filename, readcnt, s.cols);
        fprintf(stderr,
                "       check if size (%d, %d) is correct for '%s'\n",
                s.rows, s.cols, filename);
        exit(IOERR);
    }
  }
  fclose(ifile);

  game(&s, max_gens);
  printf("\nGlobal Checksum after %ld generations: %ld\n", s.generation, s.checksum);

  write_bmp(output_filename, &s);
  printf("\nFinal state dumped to %s\n", output_filename);
  
  print_state(&s, "output", gsize);

  free_state(&s);
}

void game(state * s, int max_gens)
{
  long sum_gendiff = 0.;
  while ((!max_gens && LIVE) || s->generation < max_gens)
  {
    if (s->halo)
    {
      swap_halo(s);
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

/*
 * Place bounding rows, columns and corners into adjacent halos
 */
void swap_halo(state * s)
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

void print_state(state * s, const char * filename, int *gsize)
{
  // This function writes the space of state "s" to a file
  // The output file must be also a valid input file
  // If GoL is run with '0' iterations, the output file should be equal to input file
  FILE * ofile = fopen(filename, "w");
  int halo = s->halo;
  
  for (int y = halo; y < gsize[ROWS] + halo; ++y) {
    fwrite(s->space[y]+halo, sizeof(char), gsize[COLS], ofile);
  }

  fclose(ofile);  
}
