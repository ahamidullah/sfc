#ifndef __AST_H__
#define __AST_H__

#include <stdint.h>

typedef struct ast_node ast_node;

/* hack necessary because we don't properly handle errors yet, so 
 * no way to distinguish between the end of a list and an error return
 */
#define END_STMT_LIST (ast_node *)-1

enum ast_type {
	type_expr,
	type_name,
	type_num,
	type_astmt,
	type_ifstmt,
	type_stmtlist,
	type_wstmt,
	type_fstmt,
};

struct ast_expr {
	ast_node *left;
	const char *op;
	ast_node *right;
};

struct ast_astmt {
	ast_node *lval;
	ast_node *rval;
};

struct ast_ifstmt {
	ast_node *condexpr;
	ast_node *stmtlist;
};

struct ast_wstmt {
	ast_node *condexpr;
	ast_node *stmtlist;
};

struct ast_fstmt {
	ast_node *init;
	ast_node *condexpr;
	ast_node *onloop;
	ast_node *stmtlist;
};

struct ast_stmtlist {
	ast_node *stmt;
	ast_node *next;
};

struct ast_node {
	ast_type type;
	union {
		char *name;
		int num;
		ast_expr expr;
		ast_stmtlist stmtlist;
		ast_astmt astmt;
		ast_ifstmt ifstmt;
		ast_wstmt wstmt;
		ast_fstmt fstmt;
	};
};

#endif
