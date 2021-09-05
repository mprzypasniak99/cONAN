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
	scp -r ../conan inf$(IDX)@polluks.cs.put.poznan.pl:~/mpi

get:
	scp -r inf$(IDX)@polluks.cs.put.poznan.pl:~/mpi/conan ..
