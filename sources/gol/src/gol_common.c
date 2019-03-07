#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "gol.h"

#define PRINT_GRAPHIC 1

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

long evolve(state * s, state * snew)
{
  long checksum = 0;

  for (int y = 1; y <= s->rows; y++)
  {
    for (int x = 1; x <= s->cols; x++)
    {
      int n = 0, y1, x1;

      if (s->space[y][x]) n--;
      for (y1 = y - 1; y1 <= y + 1; y1++)
      {
        for (x1 = x - 1; x1 <= x + 1; x1++)
        {
          if (s->space[y1][x1])
            n++;
        }
      }
      snew->space[y][x] = (n == 3 || (n == 2 && s->space[y][x]));
    }
  }

  for (int y = 1; y <= s->rows; y++)
    for (int x = 1; x <= s->cols; x++)
    {
      checksum += s->space[y][x] != snew->space[y][x];
    }

  memcpy(s->space[0], snew->space[0], (s->rows+2)*(s->cols+2));

  s->generation++;
  s->checksum += checksum;
  return checksum;
}

void show(state * s, int clear)
{
  printf(clear?"\033[H":"\n");
  for (int y = 1; y <= s->rows; y++) {
    for (int x = 1; x <= s->cols; x++) {
#if(PRINT_GRAPHIC)
      printf(s->space[y][x] ? "\033[07m  \033[m" : "  ");
#else
      printf("%c ", s->space[y][x] +'0');
#endif
    }
    printf(clear?"\033[E":"\n");
  }
  printf("\n\nGen: %5ld Checksum: %ld\n", s->generation, s->checksum);
  fflush(stdout);
}

void show_space(void * s, int rows, int cols, int clear, int offset)
{
  char (*space)[cols] = s;

  printf(clear?"\033[H":"\n");
  for (int y = offset; y < rows + offset; y++) {
    for (int x = offset; x < cols + offset; x++) {
#if(PRINT_GRAPHIC)
      printf(space[y][x] ? "\033[07m  \033[m" : "  ");
#else
      printf("%c ", space[y][x] +'0');
#endif
    }
    printf(clear?"\033[E":"\n");
  }
  fflush(stdout);
}

void alloc_state(state * s, int rows, int cols)
{
  s->rows      = rows;
  s->cols      = cols;

  s->space     = (char **) malloc ((s->rows+2) * sizeof(char *));
  s->space[0]  = (char *) calloc ((s->rows+2) * (s->cols+2), sizeof(char));
  for (int y=1; y<=s->rows+1; ++y)
      s->space[y] = s->space[0] + y*(s->cols+2);

  s->generation = 0;
  s->checksum = 0;
}

char * start(state * s)
{
  return s->space[1];
}

void free_state(state * s)
{
  free(s->space[0]);
  free(s->space);
}






/****************************************/


#ifdef _MPI_
/*
 * This function generates a bitmap file from a game state
 * Note that output image will be flipped vertically for simplicity
 *
 * filename   output file name
 * s          state to print
 * gsize      global 2D space size
 * psize      processes 2D distribution
 * comm       grid communicator
 */
void write_bmp(const char * filename, state * s, int * gsize, int * psize, MPI_Comm comm)
{
  MPI_Offset bmp_offset;
  MPI_Datatype mpi_filetype_t;
  MPI_File fh;

  int mpi_rank;
  int mpi_size;
  int col_padding = gsize[1] % 4;
  int bmp_size[2]  = {gsize[0], gsize[1]*3 + col_padding};
  int lbmp_size[2] = {s->rows, bmp_size[1] / psize[1]};

  assert(s->rows == bmp_size[0] / psize[0]);

  if (col_padding)
  {
    if (!mpi_rank)
      printf("Error writing bmp file. Current algorithm works only for widths multiple of 4\n");
    return;
  }

  MPI_Comm_rank(comm, &mpi_rank);
  MPI_Comm_size(comm, &mpi_size);

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
  for (int y = 1; y <= s->rows; ++y) {
    for (int x = 1; x <= s->cols; ++x) {
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

void write_bmp_seq(const char * filename, state * s)
{
  int gsize[2] = {s->rows, s->cols};
  int col_padding = gsize[1] % 4;
  int bmp_size[2]  = {gsize[0], gsize[1]*3 + col_padding};
  int lbmp_size[2] = {s->rows, bmp_size[1]};

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
  for (int y = s->rows; y > 0 ; --y) {
    char * outptr = bmp_space;
    for (int x = 1; x <= s->cols; ++x) {
      int rgb = s->space[y][x]?0:255;
      *outptr++ = rgb;
      *outptr++ = rgb;
      *outptr++ = rgb;
    }
    fwrite(bmp_space, 1, lbmp_size[1], fh);
  }

  fclose(fh);

  free(bmp_space);
}
