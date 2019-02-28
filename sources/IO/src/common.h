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

extern const char *input_filename;
extern const char *output_filename;
extern const int gsizes[2];

void print_matrix(int * matrix, int height, int width);
int f(int x);

#endif
