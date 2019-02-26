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

int main(int argc, char **argv)
{
  int h,w;
  int *mat;
  FILE * fd;

  if (argc < 4)
  {
    printf("Format: %s FILE ROWS COLS\n", argv[0]);
    return 1;
  }

  fd = fopen(argv[1], "w+");

  h = atoi(argv[2]);
  w = atoi(argv[3]);
  mat = (int *) malloc( h * w * sizeof(int) );

  for (int i = 0; i < h*w; ++i)
    mat[i] = i % INT_MAX;

#if(0)
    for (int y=0; y<h; ++y)
    {
      for (int x=0; x<w; ++x)
        printf("%4d ", mat[y*w + x]);
      printf("\n");
    }
#endif

  rewind(fd);

  size_t ret = fwrite(mat, sizeof(int), h*w, fd);

  printf("Dumped %ld values to %s\n", ret, argv[1]);

  fclose(fd);

  return 0;
}
