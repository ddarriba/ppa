#include "common.h"

const char *input_filename  = INPUT_FN;
const char *output_filename = OUTPUT_FN;
const int gsizes[2]         = {MATRIX_H, MATRIX_W};

void print_matrix(int * m, int h, int w)
{
  /* print */
  for (int y=0; y<h; ++y)
  {
    for (int x=0; x<w; ++x)
    {
      printf("%4d ", m[y*w + x]);
    }
    printf("\n");
  }
  fflush(stdout);
}

int f(int x)
{
  return x + 1;
}
