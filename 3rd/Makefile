.PHONY: all clean

CC=gcc
CFLAGS=-Werror -Wall -Wextra -pedantic -std=c11

all:sim

sim: mips32.h elf.o sim.c mem.o
	 $(CC) $(CFLAGS) -g -o sim elf.o mem.o sim.c


elf.o: elf.c elf.h
	$(CC) $(CFLAGS) -c elf.c

mem.o: mem.c mem.h
	$(CC) $(CFLAGS) -c -g mem.c

clean:
	rm -f sim
	rm -f *.o