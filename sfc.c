#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "ast.h"

typedef enum { num_tk, add_tk, sub_tk, eof_tk, _err_tk } token_type;

token_type lookahead_type;
char *lookahead;
int max_lookahead_sz;
FILE *file;
ast_node *ast;

int
isdelim(char c)
{
	if (c == '+' || c == '-' || c == EOF || isspace(c))
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

token_type
nexttok(void)
{
	char c;
	token_type ret = _err_tk;

	while (isspace(c = getc(file)))
		;
	lookahead[0] = '\0';
	lookahead_append(c);
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
			lookahead_append(c);
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

ast_node *
create_node(ast_type type)
{
	ast_node *n = malloc(sizeof(ast_node));
	n->type = type;
	return n;
}

ast_node *
constnum(void)
{
	char *last_look = malloc(strlen(lookahead)+1);
	last_look = strcpy(last_look, lookahead);
	if (!accept(num_tk))
		expected("number");
	ast_node *n = create_node(type_constnum);
	n->ts_data.constnum = atoi(last_look);
	printf("%s\n", last_look);
	return n;
}

ast_node *expr_prime(ast_node *);

ast_node *
expr(void)
{
	ast_node *cnum = constnum();
	ast_node *n = expr_prime(cnum);
	expect(eof_tk);
	return n;
}

int
eps_check(token_type tk)
{
	if (lookahead_type == tk)
		return 1;
	return 0;
}

ast_node *
expr_prime(ast_node *constant_num)
{
	ast_node *n;

	if (eps_check(eof_tk)) {
		return constant_num;
	}

	n = malloc(sizeof(ast_node));

	if (accept(add_tk))
		n->ts_data.expr.op = ast_add;
	else if (accept(sub_tk))
		n->ts_data.expr.op = ast_sub;
	else {
		expected("plus or minus sign");
	}
	n->type = type_expr;
	n->ts_data.expr.left = constant_num;
	n->ts_data.expr.right = expr();
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
			else
				printf("?\n");
			printf("Expression right child:\n");
			print_ast(n->ts_data.expr.right);
			break;
		case type_constnum:
			printf("%d\n", n->ts_data.constnum);
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
	expr ->  constnum expr_prime
	expr_prime -> + expr
		|     - expr
		|     eps
	*/
	file = fopen(argv[1], "r");
	lookahead = malloc(1);
	lookahead[0] = '\0';
	max_lookahead_sz = 1;
	lookahead_type = nexttok();
	ast = expr();
	print_ast(ast);
	return 0;
}
