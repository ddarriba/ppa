#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>

#define INPUT_FN  "data/integers.input"
#define OUTPUT_FN "integers.output"

#define MATRIX_H 16
#define MATRIX_W 16

#define ERROR_ARGS 1
#define ERROR_DIM  2
#define ERROR_IO   3
#define ERROR_CONF 4

typedef float mat_t;
#define MAT_FORMAT "%6.2f "

void print_matrix(mat_t * matrix, int height, int width);
int f(int x);

#endif
