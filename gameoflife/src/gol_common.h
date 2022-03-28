#define BOUND_VERTICAL   0
#define BOUND_HORIZONTAL 1
#define BOUND_UP    0
#define BOUND_DOWN  1
#define BOUND_LEFT  0
#define BOUND_RIGHT 1

#define MAX_PRINTABLE_COLS 80 /* max cols displayed */
#define MAX_PRINTABLE_ROWS 40 /* max rows displayed */

#define DEFAULT_GENS    750
#define DEFAULT_IFILE   "data/gol_grow_40_80.input"
#define DEFAULT_OFILE   "gol.output.bmp"
#define DEFAULT_HEIGHT  40
#define DEFAULT_WIDTH   80

#define ROWS 0
#define COLS 1

#ifdef _MPI_
#include <mpi.h>
#endif

typedef struct {
  int   rows;       	/* no. of rows in grid */
  int   cols;       	/* no. of columns in grid */
  char **space;    	/* a pointer to a list of pointers for storing
                     	   a dynamic NxM grid of cells */
  char *s_temp;    	/* temporary space for the evolution process */
  long generation;
  long checksum;
  int halo;
} state;

/**
 * parse the input arguments
 * @param  argc             [input]  argument count
 * @param  argv             [input]  argument values
 * @param  filename         [output] input filename
 * @param  gsize            [output] size of the state space
 * @param  max_gens         [output] maximum number of generations
 * @param  output_filename  [output] output filename for bmp file
 * @return 1 if OK, 0 otherwise
 */
int parse_arguments(int argc, char *argv[], char **filename, int *gsize, int *max_gens, char **output_filename);

/**
 * compute the next generation for state `s`
 * @param  s     [input/output] current state to evolve
 * @param  snew  auxiliary state with same size as 's'
 * @return       checksum for generation transition
 */
long evolve(state * s);

/**
 * allocates a new state
 * @param s    [output] allocated state
 * @param rows number of rows
 * @param cols number of columns
 * @param halo boolean. If TRUE, allocates extra halo rows/columns
 */
void alloc_state(state * s, int rows, int cols, int halo);

/**
 * free memory allocated for state `s`
 * @param s [description]
 */
void free_state(state * s);

/**
 * display a state
 * @param s     state to display
 * @param clear if TRUE, clears the screen (useful for LIVE view)
 */
void show(state * s, int clear);

/**
 * display a space matrix. Similar to `show``
 * @param space  space to display
 * @param rows   number of rows
 * @param cols   number of columns
 * @param clear  if TRUE, clears the screen (useful for LIVE view)
 * @param offset halo rows/columns on each side
 */
void show_space(void * space, int rows, int cols, int clear, int offset);

#ifdef _MPI_
/**
 * Create a bmp file out of a state in parallel.
 * Processes must be arranged in a 2D grid.
 *
 * @param filename output filename (.bmp)
 * @param s        state containing the space to display
 * @param gsize    dimensions of the complete state
 * @param psize    dimensions of the processes grid
 * @param comm     intracommunicator for processes
 */
void write_bmp_mpi(const char * filename, state * s, int * gsize, int * psize, MPI_Comm comm);
#endif

/**
 * Create a bmp file out of a state
 *
 * @param filename output filename (.bmp)
 * @param s        state containing the space to display
 */
void write_bmp(const char * filename, state * s);

void write_bmp_seq_matrix(const char * filename, char ** space, int rows, int cols, int halo);
