CC = gcc

CFLAGS = -O0 -g -std=c89 -Wall -Wextra -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wstrict-overflow -Wwrite-strings

all: sfc

sfc: sfc.o
	$(CC) -o sfc sfc.o

sfc.o: sfc.c
	$(CC) $(CFLAGS) -c sfc.c

clean: rm -f sfc sfc.o