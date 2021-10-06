SOURCES=$(wildcard *.c)
HEADERS=$(wildcard *.h)
FLAGS=-DDEBUG -g
IDX=141302
SUM=$(shell expr $(LIB) + $(CONANS))

all: main

main: $(SOURCES) $(HEADERS)
	mpicc $(SOURCES) $(FLAGS) -o main

clear: clean

clean:
	rm main

run: main 
	mpirun -np $(SUM) ./main $(LIB) $(CONANS) $(LAUNDRIES) $(EQ)

send:
	scp $(SOURCES) $(HEADERS) Makefile inf$(IDX)@polluks.cs.put.poznan.pl:~/mpi/cONAN/

get:
	scp -r $(FILES) inf$(IDX)@polluks.cs.put.poznan.pl:~/mpi/cONAN ..
