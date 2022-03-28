#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "gol_common.h"

#define PRINT_GRAPHIC 1
#define MIN(a,b) (a<b?a:b)

struct bmpfile_magic {
  unsigned char magic[2];
};

struct bmpfile_header {
  uint32_t filesz;
  uint16_t creator1;
  uint16_t creator2;
  uint32_t bmp_offset;
};

struct bmpinfo_header {
  uint32_t header_sz;
  int32_t width;
  int32_t height;
  uint16_t nplanes;
  uint16_t bitspp;
  uint32_t compress_type;
  uint32_t bmp_bytesz;
  int32_t hres;
  int32_t vres;
  uint32_t ncolors;
  uint32_t nimpcolors;
};

int parse_arguments(int argc, char *argv[], char **filename, int *gsize, int *max_gens, char **output_filename)
{
if (argc == 1)
  {
    *filename = DEFAULT_IFILE;
    gsize[ROWS] = DEFAULT_HEIGHT;
    gsize[COLS] = DEFAULT_WIDTH;
    *max_gens = DEFAULT_GENS;
    *output_filename = DEFAULT_OFILE;

    printf("Run default configuration:\n");
    printf("  Input file: %s\n", DEFAULT_IFILE);
    printf("  Height:     %d\n", DEFAULT_HEIGHT);
    printf("  Width:      %d\n", DEFAULT_WIDTH);
    printf("  Gens:       %d\n", DEFAULT_GENS);
  }
  else if (argc >= 5)
  {
    *filename = argv[1];
    gsize[ROWS] = atoi(argv[2]);
    gsize[COLS] = atoi(argv[3]);
    *max_gens = atoi(argv[4]);
    *output_filename = argc == 6?argv[5]:DEFAULT_OFILE;
  }
  else
    return 0;
  return 1;
}

long evolve(state * s)
{
  long checksum = 0;
  int halo = s->halo,
      h    = s->rows,
      w    = s->cols;

  assert(halo == 0 || halo == 1);

  char * temp_ptr = s->s_temp;

  for (int y = halo; y < h+halo; y++)
  {
    for (int x = halo; x < w+halo; x++)
    {
      int n = 0, y1, x1;

      if (s->space[y][x]) n--;
      for (y1 = y - 1; y1 <= y + 1; y1++)
      {
        for (x1 = x - 1; x1 <= x + 1; x1++)
        {
          if (halo)
            n += s->space[y1][x1];
          else
          {
            //Note: mod operation affects performance
            n += s->space[(y1 + h)%h][(x1 + w)%w];
          }
        }
      }
      *temp_ptr = (n == 3 || (n == 2 && s->space[y][x]));
      checksum += s->space[y][x] != *temp_ptr;
      ++temp_ptr;
    }
  }

  temp_ptr = s->s_temp;
  for (int y = halo; y < s->rows+halo; y++)
  {
    memcpy(s->space[y]+halo, temp_ptr, s->cols);
    temp_ptr += s->cols;
  }

  s->generation++;
  s->checksum += checksum;
  return checksum;
}

void show(state * s, int clear)
{
  int offset = s->halo != 0;
  int xlimit = MIN((MAX_PRINTABLE_COLS+2*s->halo), (s->cols+2*s->halo));
  int ylimit = MIN((MAX_PRINTABLE_ROWS+2*s->halo), (s->rows+2*s->halo));

  printf(clear?"\033[H":"\n");
  for (int y = offset; y < ylimit; y++) {
    for (int x = offset; x < xlimit; x++) {
#if(PRINT_GRAPHIC)
      printf(s->space[y][x] ? "\033[07m  \033[m" : "  ");
#else
      printf("%c ", s->space[y][x] +'0');
#endif
    }
    printf(clear?"\033[E":"\n");
  }
  printf("\n\nGen: %8ld Checksum: %15ld\n", s->generation, s->checksum);
  fflush(stdout);
}

void show_space(void * s, int rows, int cols, int clear, int offset)
{
  char (*space)[cols + 2*offset] = s;
  int xlimit = MIN((MAX_PRINTABLE_COLS+2*offset), (cols+2*offset));
  int ylimit = MIN((MAX_PRINTABLE_ROWS+2*offset), (rows+2*offset));

  printf(clear?"\033[H":"\n");
  if (offset)
  {
    for (int y = 0; y < ylimit; y++) {
      if (y == offset || y == rows + offset)
        printf("\n");
      for (int x = 0; x < xlimit; x++) {
        if (x == offset || x == cols + offset)
          printf("  ");
        printf("%c ", space[y][x] +'0');
      }
      printf(clear?"\033[E":"\n");
    }
  }
  else
  {
    for (int y = 0; y < rows; y++) {
      for (int x = 0; x < cols; x++) {
        printf("%c ", space[y][x] +'0');
      }
      printf(clear?"\033[E":"\n");
    }
  }
  fflush(stdout);
}

void alloc_state(state * s, int rows, int cols, int halo)
{
  int alloc_rows = halo?(rows+2):rows,
      alloc_cols = halo?(cols+2):cols;
  int ystart = halo?1:0;

  s->rows      = rows;
  s->cols      = cols;


  s->space     = (char **) malloc (alloc_rows * sizeof(char *));

#ifdef _MPI_
  MPI_Aint space_size = alloc_rows * alloc_cols * sizeof(char);
  MPI_Alloc_mem(space_size, MPI_INFO_NULL, s->space);
#else
  s->space[0]  = (char *) calloc (alloc_rows * alloc_cols, sizeof(char));
#endif
  s->s_temp =  (char *) calloc (s->rows * s->cols, sizeof(char));
  for (int y=ystart; y<alloc_rows; ++y)
      s->space[y] = s->space[0] + y*alloc_cols;

  s->generation = 0;
  s->checksum = 0;
  s->halo = halo;
}

void free_state(state * s)
{
#ifdef _MPI_
  MPI_Free_mem(s->space[0]);
#else
  free(s->space[0]);
#endif
  free(s->space);
  free(s->s_temp);
}






/****************************************/


#ifdef _MPI_
/*
 * This function generates a bitmap file from a game state
 * Note that output image will be flipped vertically for simplicity
 */
void write_bmp_mpi(const char * filename, state * s, int * gsize, int * psize, MPI_Comm comm)
{
  MPI_Offset bmp_offset;
  MPI_Datatype mpi_filetype_t;
  MPI_File fh;

  int mpi_rank;
  int mpi_size;
  int col_padding = gsize[1] % 4;
  int bmp_size[2]  = {gsize[0], gsize[1]*3 + col_padding};
  int lbmp_size[2] = {s->rows, bmp_size[1] / psize[1]};
  int halo = s->halo;

  assert(s->rows == bmp_size[0] / psize[0]);

  MPI_Comm_rank(comm, &mpi_rank);
  MPI_Comm_size(comm, &mpi_size);

  if (col_padding)
  {
    if (!mpi_rank)
      printf("Error writing bmp file. Current algorithm works only for widths multiple of 4\n");
    return;
  }

  MPI_File_open(comm, filename,
                MPI_MODE_CREATE | MPI_MODE_WRONLY,
                MPI_INFO_NULL, &fh);

  if (mpi_rank == 0)
  {

    struct bmpfile_magic magic;
    struct bmpfile_header header;
    struct bmpinfo_header bmpinfo;

    magic.magic[0] = 0x42;
    magic.magic[1] = 0x4D;

    MPI_File_write_shared(fh, &magic, sizeof(struct bmpfile_magic), MPI_BYTE, MPI_STATUSES_IGNORE);

    header.filesz = sizeof(struct bmpfile_magic) + sizeof(struct bmpfile_header) + sizeof(struct bmpinfo_header) + bmp_size[0]*bmp_size[1];
    header.creator1 = 0xFE;
    header.creator2 = 0xFE;
    header.bmp_offset = sizeof(struct bmpfile_magic) + sizeof(struct bmpfile_header) + sizeof(struct bmpinfo_header);

    MPI_File_write_shared(fh, &header, sizeof(struct bmpfile_header), MPI_BYTE, MPI_STATUSES_IGNORE);

    bmpinfo.header_sz = sizeof(struct bmpinfo_header);
    bmpinfo.height = gsize[0];
    bmpinfo.width = gsize[1];
    bmpinfo.nplanes = 1;
    bmpinfo.bitspp = 24;
    bmpinfo.compress_type = 0;
    bmpinfo.bmp_bytesz = bmp_size[0]*bmp_size[1];
    bmpinfo.hres = 2835;
    bmpinfo.vres = 2835;
    bmpinfo.ncolors = 0;
    bmpinfo.nimpcolors = 0;

    MPI_File_write_shared(fh, &bmpinfo, sizeof(struct bmpinfo_header), MPI_BYTE, MPI_STATUSES_IGNORE);
  }

  MPI_Barrier(comm);
  MPI_File_get_position_shared(fh, &bmp_offset);

  int distribs[2] = {MPI_DISTRIBUTE_BLOCK, MPI_DISTRIBUTE_BLOCK};
  int dargs[2] = {MPI_DISTRIBUTE_DFLT_DARG, MPI_DISTRIBUTE_DFLT_DARG};
  MPI_Type_create_darray(mpi_size,
                         mpi_rank,
                         2,
                         bmp_size,
                         distribs,
                         dargs,
                         psize,
                         MPI_ORDER_C,
                         MPI_BYTE,
                         &mpi_filetype_t);
  MPI_Type_commit(&mpi_filetype_t);
  MPI_File_set_view(fh, bmp_offset, MPI_CHAR, mpi_filetype_t, "native", MPI_INFO_NULL);

  char * bmp_space = (char *) malloc(lbmp_size[0] * lbmp_size[1]);

  /* compute bitmap */
  char * outptr = bmp_space;
  for (int y = halo; y < s->rows+halo; ++y) {
    for (int x = halo; x < s->cols+halo; ++x) {
      int rgb = s->space[y][x]?0:255;
      *outptr++ = rgb;
      *outptr++ = rgb;
      *outptr++ = rgb;
    }
  }

  assert(outptr - bmp_space == lbmp_size[0] * lbmp_size[1]);

  MPI_File_write(fh, bmp_space, lbmp_size[0] * lbmp_size[1] , MPI_BYTE, MPI_STATUS_IGNORE);

  MPI_File_close(&fh);

  free(bmp_space);
}
#endif

void write_bmp(const char * filename, state * s)
{
  write_bmp_seq_matrix(filename, s->space, s->rows, s->cols, s->halo);
}

/*
 * This function generates a bitmap file from a game state
 * Output image will be flipped vertically to match MPI version
 */
#define FLIP_BMP 1
void write_bmp_seq_matrix(const char * filename, char ** space, int rows, int cols, int halo)
{
  int gsize[2] = {rows, cols};
  int col_padding = gsize[1] % 4;
  int bmp_size[2]  = {gsize[0], gsize[1]*3 + col_padding};
  int lbmp_size[2] = {rows, bmp_size[1]};

  FILE *fh = fopen(filename, "w");

  struct bmpfile_magic magic;
  struct bmpfile_header header;
  struct bmpinfo_header bmpinfo;

  magic.magic[0] = 0x42;
  magic.magic[1] = 0x4D;

  fwrite(&magic, sizeof(struct bmpfile_magic), 1, fh);

  header.filesz = sizeof(struct bmpfile_magic) + sizeof(struct bmpfile_header) + sizeof(struct bmpinfo_header) + bmp_size[0]*bmp_size[1];
  header.creator1 = 0xFE;
  header.creator2 = 0xFE;
  header.bmp_offset = sizeof(struct bmpfile_magic) + sizeof(struct bmpfile_header) + sizeof(struct bmpinfo_header);

  fwrite(&header, sizeof(struct bmpfile_header), 1, fh);

  bmpinfo.header_sz = sizeof(struct bmpinfo_header);
  bmpinfo.height = gsize[0];
  bmpinfo.width = gsize[1];
  bmpinfo.nplanes = 1;
  bmpinfo.bitspp = 24;
  bmpinfo.compress_type = 0;
  bmpinfo.bmp_bytesz = bmp_size[0]*bmp_size[1];
  bmpinfo.hres = 2835;
  bmpinfo.vres = 2835;
  bmpinfo.ncolors = 0;
  bmpinfo.nimpcolors = 0;

  fwrite(&bmpinfo, sizeof(struct bmpinfo_header), 1, fh);

  char * bmp_space = (char *) malloc(lbmp_size[1]);

  /* compute bitmap */
#if(FLIP_BMP)
  for (int y = halo; y < gsize[0] - halo; ++y) {
#else
  int revhalo = halo?0:1;
  for (int y = gsize[0] - revhalo; y > -revhalo ; --y) {
#endif
    char * outptr = bmp_space;
    for (int x = halo; x < gsize[1]+halo; ++x) {
      int rgb = space[y][x]?0:255;
      *outptr++ = rgb;
      *outptr++ = rgb;
      *outptr++ = rgb;
    }
    fwrite(bmp_space, 1, lbmp_size[1], fh);
  }

  fclose(fh);

  free(bmp_space);
}
