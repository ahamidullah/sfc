#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "ast.h"

typedef enum { num_tk,
               name_tk,
               add_tk,
               sub_tk,
               mult_tk,
               div_tk,
               eof_tk,
               _err_tk,
               oparen_tk,
               cparen_tk,
             } token_type;

token_type lookahead_type;
char *lookahead;
int max_lookahead_sz;
FILE *file;
ast_node *ast;

int
isdelim(char c)
{
	if (c == '+'
	 || c == '-'
	 || c == '*'
	 || c == '/'
	 || c == '('
	 || c == ')'
	 || c == EOF
	 || isspace(c))
		return 1;
	return 0;
}

void
lookahead_append(char c)
{
	int new_len = strlen(lookahead)+1;
	if (new_len >= max_lookahead_sz) {
		max_lookahead_sz *= 2;
		lookahead = malloc(max_lookahead_sz);
	}
	lookahead[new_len-1] = c;
	lookahead[new_len] = '\0';
}

void
eat_to_delim(int (*validc_func)(int))
{
	char c;
	while (!isdelim((c = getc(file)))) {
		lookahead_append(c);
		if (!(*validc_func)(c)) lookahead_type = _err_tk;
	}
	ungetc(c, file);
	return;
}

void
nexttok(void)
{
	char c;

	lookahead_type = _err_tk;
	while (isspace(c = getc(file)))
		;
	lookahead[0] = '\0';
	lookahead_append(c);
	switch (c) {
		case '+':
			lookahead_type = add_tk;
			return;
		case '-':
			lookahead_type = sub_tk;
			return;
		case '*':
			lookahead_type = mult_tk;
			return;
		case '/':
			lookahead_type = div_tk;
			return;
		case EOF:
			lookahead_type =  eof_tk;
			return;
		case ')':
			lookahead_type = cparen_tk;
			return;
		case '(':
			lookahead_type = oparen_tk;
			return;
	}
	if (isdigit(c)) {
		lookahead_type = num_tk;
		eat_to_delim(&isdigit);
	} else if (isalpha(c)) {
		lookahead_type = name_tk;
		eat_to_delim(&isalpha);
	}
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
	abortc("Expected %s.", lhs);
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
/*
void
expect(token_type expected_type)
{
	if (lookahead_type == expected_type)
		return;
	switch(expected_type) {
		case num_tk:
			expected("integer");
			break;
		case eof_tk:
			expected("end of file");
		default:
			expected("default");
	}
}

int
accept(token_type tk)
{
	if (lookahead_type == tk) {
		lookahead_type = nexttok();
		return 1;
	}
	return 0;
}
*/

ast_node *eprime(ast_node *);

ast_node *
num(void)
{
	ast_node *n = malloc(sizeof(ast_node));
	n->type = type_num;
	n->ts_data.num = atoi(lookahead);
	nexttok();
	return n;
}

ast_node *
name(void)
{
	ast_node *n = malloc(sizeof(ast_node));
	n->type = type_name;
	n->ts_data.name = malloc(strlen(lookahead)+1);
	strcpy(n->ts_data.name, lookahead);
	nexttok();
	return n;
}

ast_node * expr(void);

ast_node *
factor(void)
{
	if (lookahead_type == oparen_tk) {
		ast_node *n;
		nexttok();
		if (!(n = expr()))
			return NULL;
		if (lookahead_type != cparen_tk)
			return NULL;
		nexttok();
		return n;
	} else if (lookahead_type == num_tk)
		return num();
	else if (lookahead_type == name_tk)
		return name();
	return NULL;
}

ast_node *
tprime(ast_node *prev_factorn)
{
	ast_node *n, *tprimen, *factorn;

	if (lookahead_type == eof_tk || lookahead_type == add_tk || lookahead_type == sub_tk || lookahead_type == cparen_tk) {
		return prev_factorn;
	}

	n = malloc(sizeof(ast_node));

	if (lookahead_type == mult_tk) {
		n->ts_data.expr.op = ast_mult;
		nexttok();
	} else if (lookahead_type == div_tk) {
		n->ts_data.expr.op = ast_div;
		nexttok();
	} else {
		expected("multiplication or division sign");
	}
	if (!(factorn = factor()))
		return NULL;
	if (!(tprimen = tprime(factorn)))
		return NULL;	
	n->type = type_expr;
	n->ts_data.expr.left = prev_factorn;
	n->ts_data.expr.right = tprimen;
	return n;
}

ast_node *
term(void)
{
	ast_node *fnode;
	if ((fnode = factor()))
		return tprime(fnode);
	return NULL;
}

ast_node *
expr(void)
{
	ast_node *termn;
	if ((termn = term()))
		return eprime(termn);
	return NULL;
}

/* combine eprime and tprime? */
ast_node *
eprime(ast_node *prev_termn)
{
	ast_node *n, *eprimen, *termn;

	if (lookahead_type == eof_tk || lookahead_type == cparen_tk) {
		return prev_termn;
	}

	n = malloc(sizeof(ast_node));

	if (lookahead_type == add_tk) {
		n->ts_data.expr.op = ast_add;
		nexttok();
	} else if (lookahead_type == sub_tk) {
		n->ts_data.expr.op = ast_sub;
		nexttok();
	} else {
		expected("plus or minus sign");
	}
	if (!(termn = term()))
		return NULL;
	if (!(eprimen = eprime(termn)))
		return NULL;	
	n->type = type_expr;
	n->ts_data.expr.left = prev_termn;
	n->ts_data.expr.right = eprimen;
	return n;
}

void
print_ast(ast_node *n)
{
	switch(n->type) {
		case type_expr:
			printf("Expression left child:\n");
			print_ast(n->ts_data.expr.left);
			if (n->ts_data.expr.op == ast_add)
				printf("+\n");
			else if (n->ts_data.expr.op == ast_sub)
				printf("-\n");
			else if (n->ts_data.expr.op == ast_div)
				printf("/\n");
			else if (n->ts_data.expr.op == ast_mult)
				printf("*\n");			
			else
				printf("?\n");
			printf("Expression right child:\n");
			print_ast(n->ts_data.expr.right);
			break;
		case type_num:
			printf("%d\n", n->ts_data.num);
			break;
		case type_name:
			printf("%s\n", n->ts_data.name);
			break;
		default:
			break;
	}
	return;
}

int
main(int argc, char **argv)
{
	/*
	expr       -> term eprime
	eprime     -> | + term eprime
	              | - term eprime
	              | eps
	term       -> factor tprime
	tprime     -> * factor tprime
	              | / factor tprime
	              | eps
	factor     -> ( expr )
	              | num
	              | name
	*/
	file = fopen(argv[1], "r");
	lookahead = malloc(1);
	lookahead[0] = '\0';
	max_lookahead_sz = 1;
	nexttok();
	ast = expr();
	print_ast(ast);
	return 0;
}
