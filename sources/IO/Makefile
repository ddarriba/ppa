CFLAGS = -Wall -pedantic -O3
CC = gcc
MPICC = mpicc

all:
	mkdir -p bin
	$(CC) $(CFLAGS) -o bin/io_gen src/io_gen.c
	$(MPICC) $(CFLAGS) -o bin/001_io src/001_io.c
	$(MPICC) $(CFLAGS) -o bin/002_io src/002_io.c -lm
	$(MPICC) $(CFLAGS) -o bin/003_io src/003_io.c
	$(MPICC) $(CFLAGS) -o bin/004_io src/004_io.c
	$(MPICC) $(CFLAGS) -o bin/004b_io src/004b_io.c
	$(MPICC) $(CFLAGS) -o bin/005_io src/005_io.c
	$(CC) $(CFLAGS) -o bin/gol_gen src/gol_gen.c
	$(MPICC) $(CFLAGS) -o bin/001_gol src/001_gol.c src/gol.c
	$(MPICC) $(CFLAGS) -o bin/002_gol src/002_gol.c src/gol.c -lm
	$(MPICC) $(CFLAGS) -o bin/003_gol src/003_gol.c src/gol.c -lm

002:
	mpirun -np 4 bin/002_io
