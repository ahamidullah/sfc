#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "ast.h"

typedef enum {
	_err_tk = 0,
	num_tk,
	name_tk,
	add_tk,
	sub_tk,
	mult_tk,
	div_tk,
	eof_tk,
	oparen_tk,
	cparen_tk,
	asg_tk,
	semi_tk,
	obrace_tk,
	cbrace_tk,
	if_tk,
} token_type;

token_type lookahead_type;
char *lookahead;
int max_lookahead_sz;
FILE *file;
ast_node *ast;

/*bad. temp...*/
#define END_STMT_LIST -1

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
	 || isspace(c)
	 || c == '='
	 || c == ';'
	 || c == '{'
	 || c == '}')
		return 1;
	return 0;
}

void
lookahead_append(char c)
{
	int new_len = strlen(lookahead)+1;
	if (new_len >= max_lookahead_sz) {
		max_lookahead_sz *= 2;
		lookahead = realloc(lookahead, max_lookahead_sz);
	}
	lookahead[new_len-1] = c;
	lookahead[new_len] = '\0';
}

int
try_till_delim(int (*validchar_func)(int))
{
	char c;
	int is_tok = 1;
	while (!isdelim((c = getc(file)))) {
		lookahead_append(c);
		if (!(*validchar_func)(c)) is_tok = 0;
	}
	ungetc(c, file);
	return is_tok;
}

int
isvarc(int c)
{
	return (isalnum(c) || c == '_');
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
		case ':':
			c = getc(file);
			lookahead_append(c);
			if (c == '=')
				lookahead_type = asg_tk;
			return;
		case ';':
			lookahead_type = semi_tk;
			return;
		case '{':
			lookahead_type = obrace_tk;
			return;
		case '}':
			lookahead_type = cbrace_tk;
			return;
	}
	if (isdigit(c)) {
		lookahead_type = num_tk;
		if (!try_till_delim(&isdigit))
			lookahead_type = _err_tk;
	} else if (isalpha(c)) {
		if (!try_till_delim(&isvarc))
			lookahead_type = _err_tk;
		else if (!strcmp(lookahead, "if"))
			lookahead_type = if_tk;
		else
			lookahead_type = name_tk;
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

void
expect(token_type expected_type)
{
	if (lookahead_type == expected_type) {
		nexttok();
		return;
	}
	switch(expected_type) {
		case num_tk:
			expected("integer");
			break;
		case eof_tk:
			expected("end of file");
		case name_tk:
			expected("variable name");
		case add_tk:
			expected("plus sign");
		case sub_tk:
			expected("minus sign");
		case mult_tk:
			expected("multiplication sign");
		case div_tk:
			expected("division sign");
		case oparen_tk:
			expected("open parenthesis");
		case cparen_tk:
			expected("closing parenthesis");
		case asg_tk:
			expected("assignment operator");
		case semi_tk:
			expected("semicolon");
		case obrace_tk:
			expected("opening brace");
		case cbrace_tk:
			expected("closing brace");
		default:
			expected("default");
	}
}

/*
void
accept(token_type tk)
{
	nexttok();
	return;
}
*/

ast_node *eprime(ast_node *);

ast_node *
num(void)
{
	ast_node *n = malloc(sizeof(ast_node));
	n->type = type_num;
	n->ts_data.num = atoi(lookahead);
	expect(num_tk);
	return n;
}

ast_node *
name(void)
{
	ast_node *n = malloc(sizeof(ast_node));
	n->type = type_name;
	n->ts_data.name = malloc(strlen(lookahead)+1);
	strcpy(n->ts_data.name, lookahead);
	expect(name_tk);
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
		expect(cparen_tk);
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

	if (lookahead_type == semi_tk || lookahead_type == add_tk || lookahead_type == sub_tk || lookahead_type == cparen_tk) {
		return prev_factorn;
	}

	n = malloc(sizeof(ast_node));

	if (lookahead_type == mult_tk) {
		nexttok();
		n->ts_data.expr.op = ast_mult;
	} else if (lookahead_type == div_tk) {
		nexttok();
		n->ts_data.expr.op = ast_div;
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
	if ((termn = term())) {
		return eprime(termn);
	}
	return NULL;
}

/* combine eprime and tprime? */
ast_node *
eprime(ast_node *prev_termn)
{
	ast_node *n, *eprimen, *termn;

	if (lookahead_type == semi_tk || lookahead_type == cparen_tk) {
		return prev_termn;
	}

	n = malloc(sizeof(ast_node));

	if (lookahead_type == add_tk) {
		nexttok();
		n->ts_data.expr.op = ast_add;
	} else if (lookahead_type == sub_tk) {
		nexttok();
		n->ts_data.expr.op = ast_sub;
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

ast_node *
create_node(ast_type t)
{
	ast_node *n = malloc(sizeof(ast_node));
	n->type = t;
	return n;
}

ast_node *
astmt(void)
{
	ast_node *n = create_node(type_astmt), *namen, *exprn;

	if (!(namen = name()))
		return NULL;
	n->ts_data.astmt.lval = namen;
	expect(asg_tk);
	if (!(exprn = expr()))
		return NULL;
	n->ts_data.astmt.rval = exprn;
	expect(semi_tk);
	return n;
}

ast_node *
condexpr(void)
{
	/*just an expr for now...*/
	return expr();
}

ast_node *ifstmt(void);
ast_node *astmt(void);

ast_node *
stmt(void)
{
	if (lookahead_type == if_tk) {
		return ifstmt();
	} else if (lookahead_type == name_tk) {
		return astmt();
	}
	return NULL;
}

ast_node *
stmtlist(void)
{
	ast_node *stmtlistn = create_node(type_stmtlist);

	if (lookahead_type == eof_tk || lookahead_type == cbrace_tk)
		return END_STMT_LIST;
	if (!(stmtlistn->ts_data.stmtlist.stmt = stmt()))
		return NULL;
	if(!(stmtlistn->ts_data.stmtlist.next = stmtlist()))
		return NULL;
	return stmtlistn;	
}

ast_node *
ifstmt(void)
{
	ast_node *n = create_node(type_ifstmt);
	expect(if_tk);
	expect(oparen_tk);
	if (!(n->ts_data.ifstmt.condexpr = condexpr()))
		return NULL;
	expect(cparen_tk);
	expect(obrace_tk);
	if (!(n->ts_data.ifstmt.stmtlist = stmtlist()))
		return NULL;
	expect(cbrace_tk);
	return n;
}

#define ast_printf(tab_depth, ...)\
{\
	int i;\
	for (i = 0; i < tab_depth; i++)\
		printf("\t");\
	printf(__VA_ARGS__);\
}

void
print_ast(ast_node *n, int depth)
{
	switch(n->type) {
		case type_expr:
			depth++;
			ast_printf(depth, "Expression left child:\n");

			print_ast(n->ts_data.expr.left, depth);
			if (n->ts_data.expr.op == ast_add) {
				ast_printf(depth, "+\n");
			} else if (n->ts_data.expr.op == ast_sub) {
				ast_printf(depth, "-\n");
			} else if (n->ts_data.expr.op == ast_div) {
				ast_printf(depth, "/\n");
			} else if (n->ts_data.expr.op == ast_mult) {
				ast_printf(depth, "*\n");			
			} else {
				ast_printf(depth, "?\n");
			}
			ast_printf(depth, "Expression right child:\n");

			print_ast(n->ts_data.expr.right, depth);
			break;
		case type_num:
			ast_printf(depth, "%d\n", n->ts_data.num);
			break;
		case type_name:
			ast_printf(depth, "%s\n", n->ts_data.name);
			break;
		case type_astmt:
			ast_printf(depth, "astmt:\n");
			ast_printf(depth, "var name:\n");
			print_ast(n->ts_data.astmt.lval, depth);
			ast_printf(depth, "expr:\n");
			print_ast(n->ts_data.astmt.rval, depth);
			break;
		case type_stmtlist:
			for (; n != END_STMT_LIST; n = n->ts_data.stmtlist.next) {
				print_ast(n->ts_data.stmtlist.stmt, depth);
			}
			break;
		case type_ifstmt:
			ast_printf(depth, "if stmt cond:\n");
			print_ast(n->ts_data.ifstmt.condexpr, depth);
			ast_printf(depth, "stmtlist\n");
			print_ast(n->ts_data.ifstmt.stmtlist, depth);
		default:
			break;
	}
	return;
}

int
main(int argc, char **argv)
{
	/*
	stmtlist   -> stmt stmtlist
	              | eps
	stmt       -> astmt;
	              | ifstmt
	astmt      -> var := expr
	ifstmt     -> if (condexpr) { stmtlist }
	expr       -> term eprime
	eprime     -> + term eprime
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
	ast = stmtlist();
	print_ast(ast, -1);
	return 0;
}
