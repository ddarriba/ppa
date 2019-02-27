/*
 * MPI IO example 1
 *
 * Read a text file concurrently as a grid
 * using collective IO operations
 */
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>

#define INF_GROWTH 2
#define BELL       1
#define RANDOM     0

#define PATTERN INF_GROWTH
//#define PATTERN BELL
//#define PATTERN RANDOM

#define INF_GROWTH_H 8
#define INF_GROWTH_W 8

#define BELL_GUN_H 24
#define BELL_GUN_W 51

const char infinity_growth[INF_GROWTH_H][INF_GROWTH_W] = {
"........",
"......O.",
"....O.OO",
"....O.O.",
"....O...",
"..O.....",
"O.O.....",
"........"
};

const char bell_gun[BELL_GUN_H][BELL_GUN_W] = {
".......................OO........................OO",
".......................OO........................OO",
".........................................OO........",
"........................................O..O.......",
".........................................OO........",
"...................................................",
"....................................OOO............",
"....................................O.O............",
".........OO.........................OOO............",
".........OO.........................OO.............",
"........O..O.......................OOO.............",
"........O..O.OO....................O.O.............",
"........O....OO....................OOO.............",
"..........OO.OO....................................",
"...............................OO..................",
".....................OO.......O..O.................",
".....................OO........OO..................",
".................................................OO",
".................................................OO",
"...................................................",
"....OO..................O..........................",
"OO....OOOO..........OO..OO.OOO.....................",
"OO..OO.OOO..........OO....OOOO.....................",
"....O...................OO........................."
};

int main(int argc, char **argv)
{
  int h,w;
  char *mat;
  FILE * fd;

  if (argc < 4)
  {
    printf("Format: %s FILE ROWS COLS\n", argv[0]);
    return 1;
  }

  fd = fopen(argv[1], "w+");

  h = atoi(argv[2]);
  w = atoi(argv[3]);
  mat = (char *) calloc( h * w, sizeof(char) );

switch(PATTERN)
{
  case(INF_GROWTH):
    for (int i = 0, iplace = .8*h - INF_GROWTH_H; i < INF_GROWTH_H; ++i, ++iplace)
    {
      for (int j = 0, jplace = w/2; j < INF_GROWTH_W; ++j, ++jplace)
      {
        mat[iplace * w + jplace - 6*INF_GROWTH_W] = infinity_growth[i][j] == 'O';
        mat[iplace * w + jplace] = infinity_growth[INF_GROWTH_H - i - 1][j] == 'O';
        mat[iplace * w + jplace + 6*INF_GROWTH_W] = infinity_growth[i][INF_GROWTH_W-j-1] == 'O';
      }
    }
    break;

  case(BELL):
    for (int i = 0; i < BELL_GUN_H; ++i)
      for (int j = 0; j < BELL_GUN_W; ++j)
      {
        mat[i * w + w/10 + j] = bell_gun[i][j] == 'O';
        mat[i * w + w - w/5 - j] = bell_gun[j][i] == 'O';
      }
    break;

  case(RANDOM):
    for (int i = 0; i < h*w; ++i)
      mat[i] = ((double)rand() / INT_MAX) > 0.5;
  break;
}

#if(0)
    for (int y=0; y<h; ++y)
    {
      for (int x=0; x<w; ++x)
        printf("%c", mat[y*w + x] + '0');
      printf("\n");
    }
#endif

  rewind(fd);

  size_t ret = fwrite(mat, sizeof(char), h*w, fd);

  printf("Dumped %ld values to %s\n", ret, argv[1]);

  fclose(fd);

  return 0;
}
