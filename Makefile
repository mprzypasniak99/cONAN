SOURCES=$(wildcard *.c)
HEADERS=$(SOURCES:.c=.h)
FLAGS=-DDEBUG -g
IDX=141302

all: main

main: $(SOURCES) $(HEADERS)
	mpicc $(SOURCES) $(FLAGS) -o main

clear: clean

clean:
	rm main

run: main
	mpirun -np 8 ./main

send:
	scp $(SOURCES) $(HEADERS) Makefile inf$(IDX)@polluks.cs.put.poznan.pl:~/mpi/cONAN/

get:
	scp -r $(FILES) inf$(IDX)@polluks.cs.put.poznan.pl:~/mpi/cONAN ..
