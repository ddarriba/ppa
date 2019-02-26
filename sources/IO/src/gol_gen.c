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
  mat = (char *) malloc( h * w * sizeof(char) );

  for (int i = 0; i < h*w; ++i)
    mat[i] = ((double)rand() / INT_MAX) > 0.5;
//    mat[i] = i;

#if(1)
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
