OBJS = sfc.cpp codegen.cpp
CC = g++
CFLAGS = -O0 -g -std=c++14 -Wall -Wextra -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wcast-align -Wstrict-overflow -Wwrite-strings
LFLAGS = 
OBJ_NAME = sfc

all: $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) $(LFLAGS) -o $(OBJ_NAME)

clean: rm -f $(OBJ_NAME) *.o
