#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

enum token { num_tk, add_tk, sub_tk, eof_tk, _err_tk };

enum token look;
char *lookstr;
int max_lookstr_sz;
FILE *file;

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
	char c;
	enum token ret = _err_tk;

	while (isspace(c = getc(file)))
		;
	lookstr[0] = '\0';
	lookstr_append(c);
	if (c == '+')
		return add_tk;
	if (c == '-')
		return sub_tk;
	if (c == EOF) {
		return eof_tk;
	}
	if (isdigit(c)) {
		ret = num_tk;
		while (!isdelim((c = getc(file)))) {
			lookstr_append(c);
			if (!isdigit(c)) ret = _err_tk;
		}
		ungetc(c, file);
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

void
expect(enum token expected_look)
{
	if (look == expected_look)
		return;
	switch(expected_look) {
		case num_tk:
			expected("integer");
			break;
		default:
			expected("default");
	}
}

int
accept(enum token tk)
{
	if (look == tk)
		return 1;
	return 0;
}

void
constnum(void)
{
	if (!accept(num_tk))
		expected("number");
	emitln("MOV %s %%EAX", lookstr);
	look = nexttok();
	return;
}

void expr_prime(void);

void
expr(void)
{
	constnum();
	expr_prime();
	/*expect(eof_tk);*/
	return;
}

void
expr_prime(void)
{
	if (accept(add_tk) || accept(sub_tk)) {
		look = nexttok();
		expr();
	} else if (accept(eof_tk)) {
		printf("it\n");
	} else {
		expected("plus or minus sign");
	}
	return;
}

int
main(int argc, char **argv)
{
	/* 
	expr ->  constnum expr_prime
	expr_prime -> + expr
		|        - expr
		|       eps
	*/
	file = fopen(argv[1], "r");
	lookstr = malloc(1);
	lookstr[0] = '\0';
	max_lookstr_sz = 1;
	look = nexttok();
	expr();
	return 0;
}
