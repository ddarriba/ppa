#define BOUND_VERTICAL   0
#define BOUND_HORIZONTAL 1
#define BOUND_UP    0
#define BOUND_DOWN  1
#define BOUND_LEFT  0
#define BOUND_RIGHT 1

#include <mpi.h>

typedef struct {
  int   rows;       	/* no. of rows in grid */
  int   cols;       	/* no. of columns in grid */
  char **space;    	/* a pointer to a list of pointers for storing
                     	   a dynamic NxM grid of cells */
  int generation;
} state;

long evolve(state * s, state * snew);
void alloc_state(state * s, int rows, int cols);

char * start(state * s);

void free_state(state * s);
void show(state * s, int clear);
void show_space(void * space, int rows, int cols, int clear, int offset);

void write_bmp(const char * filename, state * s, int * gsize, int * psize, MPI_Comm comm);
