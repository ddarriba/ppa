/* worker */

#include <stdio.h>
#include <mpi.h>

#define SERVICE_NAME "my_service_name"

int main(int argc, char *argv[])
{
   int mpi_size, mpi_rank;
   int global_size, global_rank;
   int parent_size;

   MPI_Comm parent_comm, global_comm, connect_comm;
   MPI_Init(&argc, &argv);

   MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
   MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

   char port_name[MPI_MAX_PORT_NAME];

   MPI_Lookup_name(SERVICE_NAME, MPI_INFO_NULL, port_name);

   printf("Connecting to port %s\n", port_name);

   MPI_Comm_connect(port_name, MPI_INFO_NULL,
                    0, MPI_COMM_WORLD, &connect_comm);


   MPI_Intercomm_merge(connect_comm,
                       1,
                       &global_comm);

   /* if the intercommunicator is no longer used, we can disconnect */
   MPI_Comm_disconnect(&connect_comm);

   MPI_Comm_rank(global_comm, &global_rank);
   MPI_Comm_size(global_comm, &global_size);

   MPI_Barrier(global_comm);
   for (int i=0; i<global_size; ++i)
   {
     if (i == global_rank)
     {
       printf("Child %d/%d\n", mpi_rank, mpi_size);
       printf("  Parent size: %d\n", parent_size);
       printf("  Global process %d/%d\n", global_rank, global_size);
     }
     MPI_Barrier(global_comm);
   }
   /*
    * Parallel code here.
    * The manager is represented as the process with rank 0 in (the remote
    * group of) MPI_COMM_PARENT.  If the workers need to communicate among
    * themselves, they can use MPI_COMM_WORLD.
    */


   MPI_Finalize();
   return 0;
}
