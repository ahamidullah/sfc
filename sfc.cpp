/*just a basic recursive descent parser for now...
* a lot of stuff is boilerplate that we should
* generate when we know how it's going to finally be laid out
*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "ast.h"

/*BOILERPLATE*/
enum token_type {
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
	while_tk,
	for_tk,
	gt_tk,
	lt_tk,
	gte_tk,
	lte_tk,
};

token_type look;
char *lookstr;
int max_lookstr_sz;
FILE *file;

struct keyword_token_map {
	const char *str;
	const token_type token;
};

const keyword_token_map keywords[] = {
	{"if", if_tk},
	{"while", while_tk},
	{"for", for_tk},
};

static
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
	 || c == '}'
	 || c == '<'
	 || c == '>')
		return 1;
	return 0;
}

/*resize lookstr if new char goes past max_lookstr_sz*/
static
void
lookstr_append(char c)
{
	int new_len = strlen(lookstr)+1;
	if (new_len >= max_lookstr_sz) {
		max_lookstr_sz *= 2;
		lookstr = (char *)realloc(lookstr, max_lookstr_sz);
	}
	lookstr[new_len-1] = c;
	lookstr[new_len] = '\0';
}

/*eat chars till delim and make sure validchar_func is true for all*/
static
int
try_till_delim(int (*validchar_func)(int))
{
	char c;
	int is_tok = 1;
	while (!isdelim((c = getc(file)))) {
		lookstr_append(c);
		if (!(*validchar_func)(c)) is_tok = 0;
	}
	ungetc(c, file);
	return is_tok;
}

/*check if character is valid variable character*/
static
int
isvarc(int c)
{
	return (isalnum(c) || c == '_');
}

/*eat chars till delim. add them to lookstr. set look to corresponding token*/
static
void
nexttok()
{
	char c;

	look = _err_tk;
	while (isspace(c = getc(file)))
		;
	lookstr[0] = '\0';
	lookstr_append(c);
	switch (c) {
		case '+':
			look = add_tk;
			return;
		case '-':
			look = sub_tk;
			return;
		case '*':
			look = mult_tk;
			return;
		case '/':
			look = div_tk;
			return;
		case EOF:
			look =  eof_tk;
			return;
		case ')':
			look = cparen_tk;
			return;
		case '(':
			look = oparen_tk;
			return;
		case ':':
			lookstr_append((c = getc(file)));
			if (c == '=')
				look = asg_tk;
			return;
		case ';':
			look = semi_tk;
			return;
		case '{':
			look = obrace_tk;
			return;
		case '}':
			look = cbrace_tk;
			return;
		case '>':
			look = gt_tk;
			c = getc(file);
			if (c == '=') {
				lookstr_append(c);
				look = gte_tk;
			} else
				ungetc(c, file);
			return;
		case '<':
			look = lt_tk;
			c = getc(file);
			if (c == '=') {
				lookstr_append(c);
				look = lte_tk;
			} else
				ungetc(c, file);
			return;
	}
	if (isdigit(c)) {
		look = num_tk;
		if (!try_till_delim(&isdigit))
			look = _err_tk;
	} else if (isalpha(c)) {
		int i, num_keywords = (sizeof(keywords)/sizeof(keywords[0]));

		if (!try_till_delim(&isvarc)) {
			look = _err_tk;
			return;
		}
		for (i = 0; i < num_keywords; i++) {
			if (!strcmp(keywords[i].str, lookstr)) {
				look = keywords[i].token;
				return;
			}
		}
		look = name_tk;
	}
	return;
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

static
void
expected(const char *lhs)
{
	abortc("Expected %s.", lhs);
}

static
void
expect(token_type expected_type)
{
	if (look == expected_type) {
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

static
ast_node *
create_node(ast_type t)
{
	ast_node *n = (ast_node *)malloc(sizeof(ast_node));
	n->type = t;
	return n;
}

static ast_node *eprime(ast_node *);

static
ast_node *
num()
{
	ast_node *n = create_node(type_num);
	n->num = atoi(lookstr);
	expect(num_tk);
	return n;
}

static
ast_node *
name(ast_type t) //get a generic name (could be type, var, function, etc.)
{
	ast_node *n = create_node(t);
	n->name = (char *)malloc(strlen(lookstr)+1);
	strcpy(n->name, lookstr);
	expect(name_tk);
	return n;
}

static ast_node * expr();

//variable name occuring in an expression
static
ast_node *
exprvar()
{
	return name(type_exprvar);
}

//variable name being assigned to
static
ast_node *
asmtvar()
{
	return name(type_asmtvar);
}

static
ast_node *
factor()
{
	if (look == oparen_tk) {
		ast_node *n;
		nexttok();
		if (!(n = expr()))
			return NULL;
		expect(cparen_tk);
		return n;
	} else if (look == num_tk)
		return num();
	else if (look == name_tk)
		return exprvar();
	return NULL;
}

static
ast_node *
tprime(ast_node *prev_factorn)
{
	ast_node *n, *tprimen, *factorn;

	if (look == semi_tk
	    || look == add_tk
	    || look == sub_tk
	    || look == cparen_tk
	    || look == gt_tk
	    || look == lt_tk
	    || look == gte_tk
	    || look == lte_tk)
		return prev_factorn;

	n = create_node(type_expr);
	if (look == mult_tk) {
		nexttok();
		n->expr.op = "imul";
	} else if (look == div_tk) {
		nexttok();
		n->expr.op = "idiv";
	} else {
		expected("multiplication or division sign");
	}
	if (!(factorn = factor()))
		return NULL;
	if (!(tprimen = tprime(factorn)))
		return NULL;	
	n->expr.left = prev_factorn;
	n->expr.right = tprimen;
	return n;
}

static
ast_node *
term()
{
	ast_node *fnode;
	if ((fnode = factor()))
		return tprime(fnode);
	return NULL;
}

static
ast_node *
cond(ast_node *left_expr)
{
	ast_node *n;
	
	if (look == semi_tk || look == cparen_tk)
		return left_expr;
	n = create_node(type_expr);
	n->expr.left = left_expr;
	if (look == gt_tk)
		n->expr.op = "GT";
	else if (look == lt_tk)
		n->expr.op = "LT";
	else if (look == gte_tk)
		n->expr.op = "GTE";
	else if (look == lte_tk)
		n->expr.op = "LTE";
	else
		expected("conditional operator");
	nexttok();
	if (!(n->expr.right = expr()))
		return NULL;
	return n;
}

static
ast_node *
expr()
{
	ast_node *termn, *eprimen;
	if (!(termn = term()))
		return NULL;	
	if ((eprimen = eprime(termn)))
		return cond(eprimen);
	return NULL;
}

/* combine eprime and tprime? */
static
ast_node *
eprime(ast_node *prev_termn)
{
	ast_node *n, *eprimen, *termn;

	if (look == semi_tk
	    || look == cparen_tk
	    || look == gt_tk
	    || look == lt_tk
	    || look == gte_tk
	    || look == lte_tk)
		return prev_termn;

	n = create_node(type_expr);
	if (look == add_tk) {
		nexttok();
		n->expr.op = "add";
	} else if (look == sub_tk) {
		nexttok();
		n->expr.op = "sub";
	} else {
		expected("plus or minus sign");
	}
	if (!(termn = term()))
		return NULL;
	if (!(eprimen = eprime(termn)))
		return NULL;
	n->expr.left = prev_termn;
	n->expr.right = eprimen;
	return n;
}

static
ast_node *
astmt()
{
	ast_node *n = create_node(type_astmt), *namen, *exprn;

	if (!(namen = asmtvar()))
		return NULL;
	n->astmt.lval = namen;
	expect(asg_tk);
	if (!(exprn = expr()))
		return NULL;
	n->astmt.rval = exprn;
	return n;
}

static
ast_node *
condexpr()
{
	/*just an expr for now...*/
	return expr();
}

static ast_node *ifstmt();
static ast_node *astmt();
static ast_node *wstmt();
static ast_node *fstmt();

static
ast_node *
stmt()
{
	if (look == if_tk) {
		return ifstmt();
	} else if (look == while_tk) {
		return wstmt();
	} else if (look == for_tk) {
		return fstmt();
	} else if (look == name_tk) {
		ast_node *n = astmt();
		expect(semi_tk);
		return n;
	}
	return NULL;
}

ast_node *
stmtlist()
{
	ast_node *stmtlistn = create_node(type_stmtlist);

	if (look == eof_tk || look == cbrace_tk)
		return END_STMT_LIST;
	if (!(stmtlistn->stmtlist.stmt = stmt()))
		return NULL;
	if(!(stmtlistn->stmtlist.next = stmtlist()))
		return NULL;
	return stmtlistn;	
}

static
ast_node *
ifstmt()
{
	ast_node *n = create_node(type_ifstmt);
	expect(if_tk);
	expect(oparen_tk);
	if (!(n->ifstmt.condexpr = condexpr()))
		return NULL;
	expect(cparen_tk);
	expect(obrace_tk);
	if (!(n->ifstmt.stmtlist = stmtlist()))
		return NULL;
	expect(cbrace_tk);
	return n;
}

static
ast_node *
wstmt()
{
	ast_node *n = create_node(type_wstmt);
	expect(while_tk);
	expect(oparen_tk);
	if (!(n->wstmt.condexpr = condexpr()))
		return NULL;
	expect(cparen_tk);
	expect(obrace_tk);
	if (!(n->wstmt.stmtlist = stmtlist()))
		return NULL;
	expect(cbrace_tk);
	return n;
}

static
ast_node *
fstmt()
{
	ast_node *n = create_node(type_fstmt);
	expect(for_tk);
	expect(oparen_tk);
	if (!(n->fstmt.init = astmt()))
		return NULL;
	expect(semi_tk);
	if (!(n->fstmt.condexpr = condexpr()))
		return NULL;
	expect(semi_tk);
	if (!(n->fstmt.onloop = astmt()))
		return NULL;
	expect(cparen_tk);
	expect(obrace_tk);
	if (!(n->fstmt.stmtlist = stmtlist()))
		return NULL;
	expect(cbrace_tk);
	return n;
}

void gen_code(ast_node *ast);

int
main(int argc, char **argv)
{
	/* grammar:
	stmtlist   -> stmt stmtlist
	              | eps
	stmt       -> astmt;
	              | ifstmt
	              | wstmt
	              | fstmt
	astmt      -> lvar := expr
	ifstmt     -> if (condexpr) { stmtlist }
	wstmt      -> while (condexpr) { stmtlist }
	fstmt      -> for (init; condexpr; onloop) { stmtlist }
	expr       -> term eprime cond
	cond       -> > expr
	              | < expr
	              | >= expr
	              | <= expr
	              | eps
	eprime     -> + term eprime
	              | - term eprime
	              | eps
	term       -> factor tprime
	tprime     -> * factor tprime
	              | / factor tprime
	              | eps
	factor     -> ( expr )
	              | num
	              | rvar
	*/
	if (argc == 1)
		abortc("no filename given");
	file = fopen(argv[1], "r");
	lookstr = (char *)malloc(1);
	lookstr[0] = '\0';
	max_lookstr_sz = 1;
	nexttok();
	ast_node *ast = stmtlist();
	if (ast)
		gen_code(ast);
	else
		printerr("ast err\n");
	return 0;
}
