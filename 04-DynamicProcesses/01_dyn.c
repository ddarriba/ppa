/*
 * Dynamic processes example 1: Spawn hello world processes
 *
 * Compile: mpicc -Wall -o 01_dyn 01_dyn.c
 *          mpicc -Wall -o aux_01_worker aux_01_worker.c
 *
 * NOTE: aux_01_worker must be compiled before running this example:
 *       
 */
#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>

int main(int argc, char *argv[])
{
   int world_size, universe_size, *universe_sizep, flag;
   MPI_Comm children_comm;           /* intercommunicator */
   
   /* by default, all spawned children will be aux_01_worker */
   char worker_program[100] = "./aux_01_worker";

   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &world_size);

   if (world_size != 1)
   {
         printf("Top heavy with management\n");
         MPI_Finalize();
         return 1;
   }

   /* UNIVERSE SIZE may contain the number of slots that have been allocated
    * in a batch scheduler. If such a value is not reachable, we take the
    * number of processes from the first argument */
   MPI_Attr_get(MPI_COMM_WORLD, MPI_UNIVERSE_SIZE,
                &universe_sizep, &flag);
   if (!flag) {
        printf("This MPI does not support UNIVERSE_SIZE.\n");
        if (argc < 2)
        {
          printf("Usage: %s NPROCS\n", argv[0]);
          MPI_Finalize();
          return 2;
        }
        universe_size = atoi(argv[1]);
        printf("Total number of processes set to %d\n", universe_size);
   }
   else
     universe_size = *universe_sizep;

   if (universe_size == 1)
   {
         printf("No room to start workers\n");
         MPI_Finalize();
         return 3;
   }

   /*
    * Now spawn the workers. Note that there is a run-time determination
    * of what type of worker to spawn, and presumably this calculation must
    * be done at run time and cannot be calculated before starting
    * the program. If everything is known when the application is
    * first started, it is generally better to start them all at once
    * in a single MPI_COMM_WORLD.
    */

   MPI_Comm_spawn(worker_program, MPI_ARGV_NULL, universe_size-1,
             MPI_INFO_NULL, 0, MPI_COMM_SELF, &children_comm,
             MPI_ERRCODES_IGNORE);

   /*
    * Parallel code here. The communicator "everyone" can be used
    * to communicate with the spawned processes, which have ranks 0,..
    * MPI_UNIVERSE_SIZE-1 in the remote group of the intercommunicator
    * "everyone".
    */

   MPI_Finalize();
   return 0;
}
