#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gol.h"

long evolve(state * s, state * snew)
{
  long checksum = 0;

  for (int y = 1; y <= s->rows; y++)
  {
    for (int x = 1; x <= s->cols; x++)
    {
      int n = 0, y1, x1;

      if (s->space[y][x]) n--;
      for (y1 = y - 1; y1 <= y + 1; y1++)
      {
        for (x1 = x - 1; x1 <= x + 1; x1++)
        {
          if (s->space[y1][x1])
            n++;
        }
      }
      snew->space[y][x] = (n == 3 || (n == 2 && s->space[y][x]));
    }
  }

  for (int y = 1; y <= s->rows; y++)
    for (int x = 1; x <= s->cols; x++)
    {
      checksum += s->space[y][x] != snew->space[y][x];
    }

  memcpy(s->space[0], snew->space[0], (s->rows+2)*(s->cols+2));

  return checksum;
}

void show(state * s, int clear)
{
  printf(clear?"\033[H":"\n");
  for (int y = 1; y <= s->rows; y++) {
    for (int x = 1; x <= s->cols; x++) {
      printf("%c ", s->space[y][x] +'0');
//      printf(s->space[y][x] ? "\033[07m  \033[m" : "  ");
    }
    printf(clear?"\033[E":"\n");
  }
  fflush(stdout);
}

void show_space(void * s, int rows, int cols, int clear, int offset)
{
  char (*space)[cols] = s;

  printf(clear?"\033[H":"\n");
  for (int y = offset; y < rows + offset; y++) {
    for (int x = offset; x < cols + offset; x++) {
//      printf("%c ", space[y][x] +'0');
      printf(space[y][x] ? "\033[07m  \033[m" : "  ");
    }
    printf(clear?"\033[E":"\n");
  }
  fflush(stdout);
}

void alloc_state(state * s, int rows, int cols)
{
  s->rows      = rows;
  s->cols      = cols;

  s->space     = (char **) malloc ((s->rows+2) * sizeof(char *));
  s->space[0]  = (char *) calloc ((s->rows+2) * (s->cols+2), sizeof(char));
  for (int y=1; y<=s->rows+1; ++y)
      s->space[y] = s->space[0] + y*(s->cols+2);
}

char * start(state * s)
{
  return s->space[1];
}

void free_state(state * s)
{
  free(s->space[0]);
  free(s->space);
}
