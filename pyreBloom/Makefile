GCC     = gcc
GCCOPTS = -O3 -Wall -g
LD      = gcc
LDOPTS  = -lhiredis

all: pyre

pyre: bloom.o main.o
	$(GCC) $(GCCOPTS) main.o bloom.o -o pyre $(LDOPTS)

main.o: main.c
	$(GCC) $(GCCOPTS) -c main.c -o main.o

bloom.o: bloom.h bloom.c
	$(GCC) $(GCCOPTS) -c bloom.c -o bloom.o

clean:
	rm -rdf *.o pyre
