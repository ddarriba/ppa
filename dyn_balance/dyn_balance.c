/*
 * Toy tool for dynamic balance.
 *
 * Run:
 *     ./dyn_balance < WORKLOAD_LIST
 *
 * Input is a file containing a list of integers that represents each job's workload
 * Number of jobs will be defined by the number of lines
 *
 * This implementation has the following properties:
 *   - Each process keeps the complete list of "jobs"
 *   - Cyclic workload distribution
 *
 * The goal is to achieve the optimal workload balance given assuming that:
 *   - Each task cannot be parallelized
 *   - Workload for each task is unknown until it finishes
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_WORKLOAD 50000
#define MAX_JOBS 20
#define WORKLOAD_MULTIPLIER 1000000

int work(int rank, int job_id, int workload)
{

  printf("[%d] Job %d: Start (%d)\n", rank, job_id, workload);
  fflush(stdout);

  for(int i=0; i<workload; i++)
    for(int j=0; j<WORKLOAD_MULTIPLIER; j++)
        asm("");

  return 0;
}

int main()
{
  int mpi_rank, mpi_size;
  int job_count = 0;
  int next_job;
  int job_queue[MAX_JOBS];
  int my_workload = 0;
  int *workloads = 0;

  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  if (!mpi_rank)
  {
    workloads = (int *) calloc (mpi_size, sizeof(int));
    int sum_workload = 0;
    for (size_t i=0; i<MAX_JOBS; ++i)
    {
      job_queue[job_count] = (MAX_WORKLOAD - i*(MAX_WORKLOAD/MAX_JOBS))/(i%2+1);
      sum_workload += job_queue[job_count];
      ++job_count;
    }

    /* shuffle */
    srand(12345); /* fix random seed */
    for (size_t sreps = 0; sreps < 0; ++sreps)
    for (size_t i = 0; i < job_count - 1; ++i)
    {
      size_t j = i + rand() / (RAND_MAX / (job_count - i) + 1);
      int t = job_queue[j];
      job_queue[j] = job_queue[i];
      job_queue[i] = t;
    }

    printf("Read %d jobs\n", job_count);
    printf("Total workload %d\n", sum_workload);
    printf("Average workload %.2f\n", 1.0*sum_workload/job_count);
    printf("Average workload per process %.2f\n", 1.0*sum_workload/job_count/mpi_size);
  }

  MPI_Bcast(&job_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(job_queue, job_count, MPI_INT, 0, MPI_COMM_WORLD);

  for (next_job = mpi_rank; next_job < job_count; next_job += mpi_size)
  {
      work(mpi_rank, next_job, job_queue[next_job]);
      my_workload += job_queue[next_job];
  }

  printf("Process %d done: Workload = %d\n", mpi_rank, my_workload);

  MPI_Gather(&my_workload, 1, MPI_INT, workloads, 1, MPI_INT, 0, MPI_COMM_WORLD);

  if (!mpi_rank)
  {
    int min_workload = workloads[0],
        max_workload = workloads[0];
    for (int i=0; i<mpi_size; ++i)
    {
      if (workloads[i] < min_workload)
        min_workload = workloads[i];
      else if (workloads[i] > max_workload)
        max_workload = workloads[i];
    }

    printf("\n\nMin: %d, Max: %d\n", min_workload, max_workload);
    printf("Workload balance = %lf\n", 1.0*min_workload/max_workload);
    free(workloads);
  }

  MPI_Finalize();
  return 0;
}
