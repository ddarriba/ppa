CC = gcc
MPICC = mpicc

CFLAGS = -Wall -g -O3 -std=gnu99
LFLAGS = -lm

COMMON = #src/common.c

GOL_COMMON = src/gol_common.c

BINFILES=bin/gameoflife_seq

all: $(BINFILES)

bin/%seq: src/%seq.c $(DEPS)
		@mkdir -p "$(@D)"
		$(CC) $(CFLAGS) -o $@ $< $(COMMON) $(GOL_COMMON) $(LFLAGS)

bin/%: src/%.c $(DEPS)
		@mkdir -p "$(@D)"
		$(MPICC) $(CFLAGS) -o $@ $< $(COMMON) $(GOL_COMMON) $(LFLAGS)

clean:
		@rm -rf bin
