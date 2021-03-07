CC = gcc
MPICC = mpicc

CFLAGS = -Wall -g -O3 -std=gnu99 
LFLAGS = -lm

GOL_COMMON = src/gol_common.c

BINFILES=bin/gameoflife_seq bin/gameoflife_mpi

all: $(BINFILES)

bin/%seq: src/%seq.c $(DEPS)
		@mkdir -p "$(@D)"
		$(CC) $(CFLAGS) -o $@ $< $(GOL_COMMON) $(LFLAGS)
		$(CC) $(CFLAGS) -DLIVE=1 -o $@_LIVE $< $(GOL_COMMON) $(LFLAGS)

bin/%: src/%.c $(DEPS)
		@mkdir -p "$(@D)"
		$(MPICC) $(CFLAGS) -D_MPI_ -o $@ $< $(GOL_COMMON) $(LFLAGS)

clean:
		@rm -rf bin
