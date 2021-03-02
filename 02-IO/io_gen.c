/*
 * Input file generator
 *
 * This tool generates a binary file with a sequence of COUNT integer values
 *
 * Compile: gcc -Wall -o io_gen io_gen.c common.c
 *
 * Run: io_gen FILENAME COUNT [create|validate]
 *
 *      FILENAME: I/O binary files
 *      COUNT:    number of integers
 *      create:   create the binary file (FILENAME is an output file)
 *      validate: validate the binary file (FILENAME is an input file) using the function defined in common.h
 */
#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>

#define PRINT_VALUES 1
#define BLOCK_SIZE   1000000

typedef int val_t;

int main(int argc, char **argv)
{
  size_t count, iocount=0, ecount=0;
  val_t *v;
  FILE * fd;
  int do_create = 0;

  if (argc != 4)
  {
    printf("Usage: %s FILENAME COUNT [create|validate]\n", argv[0]);

    return ERROR_ARGS;
  }

  do_create = !strcmp(argv[3], "create");

  if (do_create)
  {
    if (!(fd = fopen(argv[1], "w+")))
    {
       printf("Error %d\n", errno);
       perror("Cannot open file for writing");
       return ERROR_IO;
    }
  }
  else
  {
    if (strcmp(argv[3], "validate"))
    {
      printf("Error: Invalid operation %s\n", argv[3]);
      return ERROR_ARGS;
    }
    if (!(fd = fopen(argv[1], "r")))
    {
       printf("Error %d\n", errno);
       perror("Cannot open file for reading");
       return ERROR_ARGS;
    }
  }

  count = atol(argv[2]);

  v = (val_t *) malloc( count * sizeof(val_t) );

  if (do_create)
  {
    for (size_t i = 0; i < count; i+=BLOCK_SIZE)
    {
      size_t limit = (i+BLOCK_SIZE < count)
        ? BLOCK_SIZE
        : count - i;

      for (size_t j = 0; j < limit; ++j)
        v[j] = j % INT_MAX;

      size_t ret = fwrite(v, sizeof(val_t), limit, fd);
      iocount += ret;
      if (ret != limit)
      {
        perror("Error writing");
        return ERROR_IO;
      }
    }
    printf("Dumped %ld values to %s\n", iocount, argv[1]);
  }
  else
  {
    for (size_t i = 0; i < count; i+=BLOCK_SIZE)
    {
      size_t limit = (i+BLOCK_SIZE < count)
        ? BLOCK_SIZE
        : count - i;

      for (size_t j = 0; j < limit; ++j)
        v[j] = j % INT_MAX;

      size_t ret = fread(v, sizeof(val_t), limit, fd);
      iocount += ret;
      if (ret != limit)
      {
        perror("Error reading");
        return 1;
      }

      for (size_t j = 0; j < limit; ++j)
      {
        if (v[j] != f(j+i))
        {
          ++ecount;
          fprintf(stderr, "Error at position %ld. Stored value is %d instead of %d\n", j+i, v[j], f(j+i));
        }
      }
    }
    printf("Read %ld values from %s\n   %ld errors found\n", iocount, argv[1], ecount);
  }

  free(v);
  fclose(fd);

  return 0;
}
