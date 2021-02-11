/*
 * Shows control and performance variables
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <mpi.h>


int main(int argc, char **argv)
{

  /* mpi_defs */
  int mpi_rank, mpi_size;

  int thread_level;

  MPI_Init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  MPI_T_init_thread(MPI_THREAD_SINGLE, &thread_level);

  int cvar_count, pvar_count;
  MPI_T_cvar_get_num(&cvar_count);

  if (mpi_rank > 0)
  {
    MPI_Finalize();
    return(0);
  }
  else if (mpi_size > 1)
  {
    printf("Only root process will continue\n");
  }

  printf("There are %d control variables\n", cvar_count);

  for (int cvar_index=0; cvar_index<cvar_count; ++cvar_index)
  {
    char cvar_name[200], cvar_desc[1000];
    int cvar_name_len = 200, cvar_desc_len = 1000,
        cvar_binding, cvar_scope, cvar_verbosity;
    MPI_Datatype cvar_datatype;
    MPI_T_enum cvar_enumtype;

    MPI_T_cvar_get_info(cvar_index, cvar_name, &cvar_name_len,
           &cvar_verbosity, &cvar_datatype, &cvar_enumtype,
           cvar_desc, &cvar_desc_len, &cvar_binding, &cvar_scope);

    printf("Variable:    [%d] %s\n", cvar_index, cvar_name);
    printf("Description: %s\n", cvar_desc);
    printf("Binding:     ");
    switch(cvar_binding)
    {
      case MPI_T_BIND_NO_OBJECT:
        printf("No object. Entire MPI process\n");
        break;
      case MPI_T_BIND_MPI_COMM:
        printf("MPI Communicators\n");
        break;
      case MPI_T_BIND_MPI_DATATYPE:
        printf("MPI Datatype\n");
        break;
      case MPI_T_BIND_MPI_ERRHANDLER:
        printf("Error handlers\n");
        break;
      case MPI_T_BIND_MPI_FILE:
        printf("File handlers\n");
        break;
      case MPI_T_BIND_MPI_GROUP:
        printf("MPI groups\n");
        break;
      case MPI_T_BIND_MPI_OP:
        printf("MPI reduction operators\n");
        break;
      case MPI_T_BIND_MPI_REQUEST:
        printf("MPI requests\n");
        break;
      case MPI_T_BIND_MPI_WIN:
        printf("MPI windows (RMA)\n");
        break;
      case MPI_T_BIND_MPI_MESSAGE:
        printf("MPI message object\n");
        break;
      case MPI_T_BIND_MPI_INFO:
        printf("MPI Info objects\n");
        break;
      default:
        printf("Unknown\n");
    }
    printf("Scope:       ");
    switch(cvar_scope)
    {
      case MPI_T_SCOPE_CONSTANT:
        printf("Read-only. Constant value\n");
        break;
      case MPI_T_SCOPE_LOCAL:
        printf("Local\n");
        break;
      case MPI_T_SCOPE_ALL_EQ:
        printf("All processes must have the same value\n");
        break;
      case MPI_T_SCOPE_ALL:
        printf("All processes must have consistent values\n");
        break;
      case MPI_T_SCOPE_READONLY:
        printf("Read-only, but it can change\n");
        break;
      case MPI_T_SCOPE_GROUP:
        printf("Group of processes. Must have consistent values\n");
        break;
      case MPI_T_SCOPE_GROUP_EQ:
        printf("Group of processes. Must have the same value\n");
        break;
      default:
        printf("Unknown\n");
    }
    printf("Datatype:    ");
    switch(cvar_datatype)
    {
      case MPI_INT:
        printf("MPI_INT\n");
        break;
      case MPI_CHAR:
        printf("MPI_CHAR\n");
        break;
      case MPI_FLOAT:
        printf("MPI_FLOAT\n");
        break;
      case MPI_DOUBLE:
        printf("MPI_DOUBLE\n");
        break;
      default:
        printf("Unknown\n");
    }

    printf("\n");
  }

  MPI_T_pvar_session session;
  MPI_T_pvar_session_create(&session);

  MPI_T_pvar_get_num(&pvar_count);
  printf("There are %d performance variables\n", pvar_count);

  for (int pvar_index=0; pvar_index<pvar_count; ++pvar_index)
  {
    char pvar_name[200], pvar_desc[1000];
    int pvar_name_len = 200, pvar_desc_len = 1000,
        pvar_binding, pvar_atomic, pvar_verbosity,
        pvar_readonly, pvar_continuous, pvar_class;
    MPI_Datatype pvar_datatype;
    MPI_T_enum pvar_enumtype;

    MPI_T_pvar_get_info(pvar_index, pvar_name, &pvar_name_len,
           &pvar_verbosity, &pvar_class, &pvar_datatype, &pvar_enumtype,
           pvar_desc, &pvar_desc_len, &pvar_binding,
           &pvar_readonly, &pvar_continuous, &pvar_atomic);

    printf("Variable:    [%d] %s\n", pvar_index, pvar_name);
    printf("Description: %s\n", pvar_desc);
    printf("Class = %d, Readonly=%s, Continuous=%s, Atomic=%s\n",
           pvar_class, pvar_readonly?"T":"F",
           pvar_continuous?"T":"F", pvar_atomic?"T":"F");

    printf("\n");
  }

  MPI_T_pvar_session_free(&session);

  MPI_T_finalize();
  MPI_Finalize();

  return 0;
}
