/*
 * This file is part of the PPA distribution (https://github.com/ddarriba/ppa).
 * Copyright (c) 2021 Diego Darriba.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "common.h"

const char *input_filename  = INPUT_FN;
const char *output_filename = OUTPUT_FN;
const int gsizes[2]         = {MATRIX_H, MATRIX_W};

void print_matrix(mat_t * m, int h, int w)
{
  /* print */
  for (int y=0; y<h; ++y)
  {
    for (int x=0; x<w; ++x)
    {
      printf(MAT_FORMAT, m[y*w + x]);
    }
    printf("\n");
  }
  fflush(stdout);
}

int f(int x)
{
  return x + 1;
}
