OBJS = sfc.c codegen.c
CC = gcc
CFLAGS = -O0 -g -std=c99 -Wall -Wextra -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wcast-align -Wstrict-overflow -Wwrite-strings
LFLAGS = 
OBJ_NAME = sfc

all: $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) $(LFLAGS) -o $(OBJ_NAME)

clean: rm -f $(OBJ_NAME) *.o
