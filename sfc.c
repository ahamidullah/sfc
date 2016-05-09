#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_NUM_DIGITS 32

#define debug_print(...)\
{\
	fprintf(stderr, __VA_ARGS__);\
}

#define printerr(...)\
{\
	fprintf(stderr, "\n");\
	fprintf(stderr, __VA_ARGS__);\
	fprintf(stderr, "\n");\
}

#define abortc(...)\
{\
	printerr(__VA_ARGS__);\
	fprintf(stderr, "Aborting...\n\n");\
	exit(1);\
}

void
expected(const char *lhs)
{
	abortc("%s expected.", lhs);
}

#define emit(...)\
{\
	printf("\t");\
	printf(__VA_ARGS__);\
}

#define emitln(...)\
{\
	emit(__VA_ARGS__);\
	emit("\n");\
}

void
flushin(void)
{
	int c;
	while ((c = getchar()) != '\n' && c != EOF)
		;
	return;
}

int
ctoi(int c)
{
	return c - '0';
}

int
getnum(void)
{
	char str[MAX_NUM_SZ];
	int num = 0;
	while ((c = getchar()) != '\n' && c != EOF) {
		if(!isdigit(c))
			expected("Integer");
		num += ctoi(c);
	}
	debug_print("%d", num);
	return num;
}

void
getexpr(void)
{
	int num = getnum();
	emitln("MOV %d %%EAX", num);
	return;
}

int
main(int argc, char **argv)
{
	while (1) {
		getexpr();
	}
	return 0;
}