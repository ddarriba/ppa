/*
 * Shows control and performance variables
 * Try this with several MPI implementations
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
  void * cvar_value = malloc(4096);

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
    char cvar_name[200], cvar_desc[1000], cvar_dt_name[200];
    int cvar_name_len = 200, cvar_desc_len = 1000,
        cvar_binding, cvar_scope, cvar_verbosity,
        cvar_dt_len;
    MPI_Datatype cvar_datatype;
    MPI_T_enum cvar_enumtype;

    MPI_T_cvar_handle cvar_handle;
    int count;

    MPI_T_cvar_get_info(cvar_index, cvar_name, &cvar_name_len,
           &cvar_verbosity, &cvar_datatype, &cvar_enumtype,
           cvar_desc, &cvar_desc_len, &cvar_binding, &cvar_scope);

    MPI_Type_get_name(cvar_datatype, cvar_dt_name, &cvar_dt_len);

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

    int handle_error = MPI_T_cvar_handle_alloc(cvar_index, NULL, &cvar_handle, &count);
    if (!handle_error)
      MPI_T_cvar_read(cvar_handle, cvar_value);
    else
      switch(handle_error)
      {
        case MPI_T_ERR_NOT_INITIALIZED:
              printf("HandleError: The MPI tool information interface is not initialized.\n");
              break;
       case MPI_T_ERR_INVALID_INDEX:
              printf("HandleError: Index is invalid or has been deleted.\n");
              break;
       case MPI_T_ERR_INVALID_HANDLE:
              printf("HandleError: The handle is invalid.\n");
              break;
       case MPI_T_ERR_OUT_OF_HANDLES:
              printf("HandleError: No more handles available.\n");
              break;
        default:
              printf("HandleError: (%d) Unknown\n", handle_error);
      }

    printf("Datatype:    %s\n", cvar_dt_name);
    if (!handle_error)
    {
      if (cvar_datatype == MPI_INT)
        printf("Value:       %d\n", ((int *)cvar_value)[0]);
      else if (cvar_datatype == MPI_CHAR)
        printf("Value:       %s\n", ((char *)cvar_value));
      else if (cvar_datatype == MPI_FLOAT)
        printf("Value:       %f\n", ((float *)cvar_value)[0]);
      else if (cvar_datatype == MPI_DOUBLE)
        printf("Value:       %lf\n", ((double *)cvar_value)[0]);
      else if (cvar_datatype == MPI_C_BOOL)
        printf("Value:       %s\n", ((int *)cvar_value)[0]?"true":"false");
    }


    printf("\n");

    MPI_T_cvar_handle_free(&cvar_handle);
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
        pvar_readonly, pvar_continuous, pvar_class,
        pvar_dt_len;;
    MPI_Datatype pvar_datatype;
    char pvar_dt_name[200];
    MPI_T_enum pvar_enumtype;

    MPI_T_pvar_get_info(pvar_index, pvar_name, &pvar_name_len,
           &pvar_verbosity, &pvar_class, &pvar_datatype, &pvar_enumtype,
           pvar_desc, &pvar_desc_len, &pvar_binding,
           &pvar_readonly, &pvar_continuous, &pvar_atomic);

    MPI_Type_get_name(pvar_datatype, pvar_dt_name, &pvar_dt_len);
    printf("Variable:    [%d] %s\n", pvar_index, pvar_name);
    printf("Description: %s\n", pvar_desc);
    printf("Class = %d, Readonly=%s, Continuous=%s, Atomic=%s\n",
           pvar_class, pvar_readonly?"T":"F",
           pvar_continuous?"T":"F", pvar_atomic?"T":"F");
    printf("Datatype:    %s\n", pvar_dt_name);

    printf("\n");
  }

  MPI_T_pvar_session_free(&session);

  MPI_T_finalize();
  MPI_Finalize();

  free(cvar_value);

  return 0;
}
