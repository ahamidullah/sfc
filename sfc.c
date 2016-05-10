#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_NUM_DIGITS 32

char look;

void
nexttok(void)
{
	while ((look = getchar()) == ' ' || look == '\t')
	  ;
	return;
}

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

int
ctoi(int c)
{
	return c - '0';
}

int
num(void)
{
	int num = 0;

	if (scanf("%d", &num) != 1)
		expected("Integer");
	debug_print("%d", num);
	nexttok();
	return num;
}

void
term(void)
{
	int n = num();
	emitln("MOV %d %%EAX", n);
	return;
}

void
add(void)
{
	term();
	emitln("ADD %%EBX %%EAX");
	return;
}

void
sub(void)
{
	term();
	emitln("SUB %%EBX %%EAX");
	return;
}

void
expr(void)
{
	term();
	emitln("MOV %%EAX %%EBX");
	while (look == '+' || look == '-') {
		if (look == '+')
			add();
		else if (look == '-')
			sub();
		else
			expected("Plus or minus");
	}
}

int
main(int argc, char **argv)
{
	while (1) {
		expr();
	}
	return 0;
}