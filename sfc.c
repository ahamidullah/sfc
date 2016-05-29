#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

enum token { num_tk, add_tk, sub_tk, eof_tk, _err_tk };

enum token look;
char *lookstr;
int max_lookstr_sz;

int
isdelim(char c)
{
	if (c == '+' || c == '-' || c == EOF || isspace(c))
		return 1;
	return 0;
}

void
lookstr_append(char c)
{
	int new_len = strlen(lookstr)+1;
	if (new_len >= max_lookstr_sz) {
		max_lookstr_sz *= 2;
		lookstr = realloc(lookstr, max_lookstr_sz);
	}
	lookstr[new_len-1] = c;
	lookstr[new_len] = '\0';
}

enum token
nexttok(void)
{
	char c = getc(stdin);
	enum token ret = _err_tk;

	lookstr_append(c);
	if (c == '+')
		return add_tk;
	if (c == '-')
		return sub_tk;
	if (c == EOF)
		return eof_tk;
	if (isdigit(c)) {
		ret = num_tk;
		while (!isdelim((c = getc(stdin)))) {
			lookstr_append(c);
			if (!isdigit(c)) ret = _err_tk;
		}
		ungetc(c, stdin);
	}
	return ret;
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

void
constnum(void)
{
	int n = 1;
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
	constnum();
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
	/* 
	expr ->  const expr_prime
	expr_prime -> + expr expr_prime
		|        - expr expr_prime
		|       eps
	*/
	lookstr = malloc(1);
	lookstr[0] = '\0';
	max_lookstr_sz = 1;
	look = nexttok();
	while (1) {
		expr();
	}
	return 0;
}
