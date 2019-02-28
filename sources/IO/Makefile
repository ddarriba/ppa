CC = gcc
MPICC = mpicc

CFLAGS = -Wall -pedantic -O3
LFLAGS = -lm

COMMON = src/common.c
GOL_COMMON = src/gol.c

BINFILES=bin/01a_io_1D_old bin/01b_io_1D_mpi bin/01c_io_1D_mpi_view \
         bin/02a_io_2D_old bin/02b_io_2D_mpi bin/03_io \
         bin/001_gol bin/002_gol bin/003_gol \
				 bin/io_gen bin/gol_gen

all: $(BINFILES)

bin/%gen: src/%gen.c $(DEPS)
		@mkdir -p "$(@D)"
		$(CC) $(CFLAGS) -o $@ $< $(COMMON) $(LFLAGS)

bin/%gol: src/%gol.c $(DEPS)
		@mkdir -p "$(@D)"
		$(MPICC) $(CFLAGS) -o $@ $< $(COMMON) $(GOL_COMMON) $(LFLAGS)

bin/%: src/%.c $(DEPS)
		@mkdir -p "$(@D)"
		$(MPICC) $(CFLAGS) -o $@ $< $(COMMON) $(LFLAGS)

clean:
		@rm -rf bin
