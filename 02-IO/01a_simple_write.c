/*
 * IO example 1a: Simple write example
 *
 * This example writes 40 characters to an output file using independent
 * write calls.
 *
 * Compile: mpicc -Wall -o 01a_simple_write.c
 * Run: no arguments required
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILESIZE 40

int main(int argc, char **argv)
{
  int rank, nprocs, char_count, bufsize,i;
  char *buf, char_to_write;
  MPI_File fh;
  MPI_Status status;

  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  bufsize = FILESIZE/nprocs;
  buf = (char *) malloc(bufsize);
  char_count = bufsize/sizeof(char);

  memcpy( &char_to_write, &rank, sizeof( char ) );

  char_to_write = char_to_write + 'A';
  for(i=0;i<char_count-1;i++)
	  buf[i]=char_to_write;
  buf[bufsize-1] = '\n';

  MPI_File_open(MPI_COMM_WORLD, "output.txt", MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);
  MPI_File_seek(fh, rank*bufsize, MPI_SEEK_SET);
  MPI_File_write(fh, buf, char_count, MPI_CHAR, &status);
  MPI_File_close(&fh);

  printf("I am process %d and I finished writing\n",rank);

  free(buf);
  MPI_Finalize();
  return 0;
}
