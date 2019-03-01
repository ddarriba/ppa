CC = gcc
MPICC = mpicc

CFLAGS = -Wall -O3 -std=c99
LFLAGS = -lm

COMMON = src/common.c

BINFILES = bin/01_shared bin/02_hybrid_comm bin/03_hybrid_mm

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
