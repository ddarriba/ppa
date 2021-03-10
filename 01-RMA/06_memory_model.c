/*
 * Prints the memory model and some other Window attributes
 *
 * Compile: mpicc -Wall -o 06_memory_model 06_memory_model.c
 * Run: no arguments required
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define WIN_SIZE 100

int work()
{
  return 0;
}

int main()
{
  int mpi_rank, mpi_size;
  int *mem_model, flag = 1;
  void *win_base, *win_size, *win_disp;
  int *mem_buffer = (int *) malloc(WIN_SIZE * sizeof(int));
  MPI_Win win;

  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  MPI_Win_create(mem_buffer, WIN_SIZE * sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

  if (!mpi_rank)
  {
    /* Call to get Memory model attribute */
    MPI_Win_get_attr(win,            // the memory window
                     MPI_WIN_MODEL,  // the attribute we are looking for
                     &mem_model,     // the returned attribute value
                     &flag);         // will return "false" here if no such attribute was found

    if (flag)
    {
      switch(*mem_model)
      {
        case MPI_WIN_UNIFIED:
          printf("Memory Model is Unified\n");
          break;
         case MPI_WIN_SEPARATE:
          printf("Memory Model is Separate\n");
          break;
        default:
          printf("Undefined Memory Model\n");
          break;
      }
    }
    else
    {
      printf("Error: The attribute does not exist in the Memory Window\n");
    }
  }

  /* Let's check also other attributes */
  MPI_Win_get_attr(win, MPI_WIN_BASE, &win_base, &flag);
  MPI_Win_get_attr(win, MPI_WIN_SIZE, &win_size, &flag);
  MPI_Win_get_attr(win, MPI_WIN_DISP_UNIT, &win_disp, &flag);

  printf("Window has size %d Bytes, the displacement unit is %d Bytes, and the base address is %p\n",
         *(int *)win_size, *(int *)win_disp, (void *)win_base);

  if (mem_buffer)
    free(mem_buffer);
  MPI_Win_free(&win);

  MPI_Finalize();
  return 0;
}
