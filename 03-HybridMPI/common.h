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
