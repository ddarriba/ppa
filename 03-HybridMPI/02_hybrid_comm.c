#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <mpi.h>

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  /* mpi_defs */
  int mpi_rank, mpi_size,   // global size/rank
      mpi_lrank, mpi_lsize; // local size/rank
  MPI_Comm shared_comm;

  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);


  MPI_Comm_split_type(MPI_COMM_WORLD,       // comm to split
                      MPI_COMM_TYPE_SHARED, // split type
                      mpi_rank,             // key for ranking
                      MPI_INFO_NULL,        // info object
                      &shared_comm);        // output comm

  MPI_Comm_rank(shared_comm, &mpi_lrank);
  MPI_Comm_size(shared_comm, &mpi_lsize);


  MPI_Group world_group, shared_group;
  int map0[mpi_size], map1[mpi_size];
  for (int i=0; i<mpi_size; i++)
    map0[i] = i;

  MPI_Comm_group (MPI_COMM_WORLD, &world_group);
  MPI_Comm_group (shared_comm, &shared_group);
  MPI_Group_translate_ranks (world_group, mpi_size, map0, shared_group, map1);

  for (int p=0; p<mpi_size; ++p)
  {
    if (p == mpi_rank)
    {
      printf("I am local process %d/%d and global process %d/%d\n",
             mpi_lrank, mpi_lsize, mpi_rank, mpi_size);
      for (int i=0; i<mpi_size; i++)
        printf("[%d] Global %d mapped to local %d\n",
          mpi_rank, map0[i], map1[i]);
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }

  MPI_Finalize();
}
