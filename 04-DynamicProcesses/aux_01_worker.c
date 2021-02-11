/* worker */

#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
   int mpi_size, mpi_rank;
   int parent_size;

   MPI_Comm parent;
   MPI_Init(&argc, &argv);
   MPI_Comm_get_parent(&parent);

   MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
   MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

   if (parent == MPI_COMM_NULL)
   {
      printf("Error: No parent!\n");
   }
   MPI_Comm_remote_size(parent, &parent_size);
   if (parent_size != 1)
   {
     printf("Error: Something's wrong with the parent\n");
   }

   /*
    * Parallel code here.
    * The manager is represented as the process with rank 0 in (the remote
    * group of) MPI_COMM_PARENT.  If the workers need to communicate among
    * themselves, they can use MPI_COMM_WORLD.
    */
   printf("Hi from child %d/%d! Parent size: %d\n", mpi_rank, mpi_size, parent_size);

   MPI_Finalize();
   return 0;
}
