/*
 * Shows control and performance variables
 * This example is implementation-dependant
 * Works with OpenMPI 4.0.3
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <mpi.h>

#define PVAR0 "mtl_psm2_rx_user_bytes"
#define PVAR0_CLASS 6
#define PVAR1 "mtl_psm2_rx_user_num"
#define PVAR1_CLASS 6


int main(int argc, char **argv)
{
  /* mpi_defs */
  int mpi_rank, mpi_size;

  int thread_level;
  int pvar_count, pvar0_id, pvar1_id, pvar0_count, pvar1_count;
  unsigned long pvar0_value, pvar1_value;
  MPI_T_pvar_handle pvar0_handle, pvar1_handle;
  int rval;

  MPI_Init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  MPI_T_init_thread(MPI_THREAD_SINGLE, &thread_level);

  MPI_T_pvar_session session;
  MPI_T_pvar_session_create(&session);

  MPI_T_pvar_get_num(&pvar_count);
  if (!pvar_count)
  {
    if (!mpi_rank)
      printf("Error: There are no performance variables\n");
    MPI_Finalize();
    return 0;
  }

  rval = MPI_T_pvar_get_index(PVAR0, PVAR0_CLASS, &pvar0_id);
  if (rval != MPI_SUCCESS)
  {
    if (rval == MPI_T_ERR_INVALID_NAME)
      printf("Error: pvar \"%s\"/ class %d does not exist\n", PVAR0, PVAR0_CLASS);
    if (rval == MPI_T_ERR_NOT_INITIALIZED)
      printf("Error: Tool information interface is not initialized\n");
    MPI_Finalize();
    return 0;
  }

  rval = MPI_T_pvar_get_index(PVAR1, PVAR1_CLASS, &pvar1_id);
  if (rval != MPI_SUCCESS)
  {
    if (rval == MPI_T_ERR_INVALID_NAME)
      printf("Error: pvar \"%s\"/ class %d does not exist\n", PVAR1, PVAR1_CLASS);
    if (rval == MPI_T_ERR_NOT_INITIALIZED)
      printf("Error: Tool information interface is not initialized\n");
    MPI_Finalize();
    return 0;
  }

void *test = malloc(400000);
printf("\nCreate handle 0\n");
  /* create handles */
  MPI_T_pvar_handle_alloc(session, pvar0_id, NULL, &pvar0_handle, &pvar0_count);
  printf("\nCreate handle 1\n");
  //MPI_T_pvar_handle_alloc(session, pvar1_id, &pvar1_value, &pvar1_handle, &pvar1_count);
printf("Done handles\n");


  //MPI_T_pvar_handle_free(session, &pvar0_handle);
  //MPI_T_pvar_handle_free(session, &pvar1_handle);
  MPI_T_pvar_session_free(&session);

  MPI_T_finalize();
  MPI_Finalize();

  return 0;
}
