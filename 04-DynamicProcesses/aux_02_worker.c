/* worker */

#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
   int mpi_size, mpi_rank;
   int global_size, global_rank;
   int parent_size;

   MPI_Comm parent_comm, global_comm;
   MPI_Init(&argc, &argv);
   MPI_Comm_get_parent(&parent_comm);

   MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
   MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

   if (parent_comm == MPI_COMM_NULL)
   {
      printf("Error: No parent!\n");
   }
   MPI_Comm_remote_size(parent_comm, &parent_size);
   if (parent_size != 1)
   {
     printf("Error: Something's wrong with the parent\n");
   }

   MPI_Intercomm_merge(
                parent_comm,
                1,
                &global_comm
              );

   MPI_Comm_rank(global_comm, &global_rank);
   MPI_Comm_size(global_comm, &global_size);

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
