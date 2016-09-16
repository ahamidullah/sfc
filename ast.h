#ifndef __AST_H__
#define __AST_H__

#include <stdint.h>

typedef struct ast_node ast_node;

/* hack necessary because we don't properly handle errors yet, so 
 * no way to distinguish between the end of a list and an error return
 */
#define END_STMT_LIST (ast_node *)-1

typedef enum ast_type {
	type_expr,
	type_name,
	type_num,
	type_astmt,
	type_ifstmt,
	type_stmtlist,
	type_wstmt,
	type_fstmt,
} ast_type;

typedef enum ast_terminal {
	ast_add,
	ast_sub,
	ast_mult,
	ast_div,
	ast_gt,
	ast_lt,
	ast_gte,
	ast_lte,
} ast_terminal;

typedef struct ast_expr {
	ast_node *left;
	ast_terminal op;
	ast_node *right;
} ast_expr;

typedef struct ast_astmt {
	ast_node *lval;
	ast_node *rval;
} ast_astmt;

typedef struct ast_ifstmt {
	ast_node *condexpr;
	ast_node *stmtlist;
} ast_ifstmt;

typedef struct ast_wstmt {
	ast_node *condexpr;
	ast_node *stmtlist;
} ast_wstmt;

typedef struct ast_fstmt {
	ast_node *init;
	ast_node *condexpr;
	ast_node *onloop;
	ast_node *stmtlist;
} ast_fstmt;

typedef struct ast_stmtlist {
	ast_node *stmt;
	ast_node *next;
} ast_stmtlist;

typedef union ast_data {
	char *name;
	int num;
	ast_expr expr;
	ast_stmtlist stmtlist;
	ast_astmt astmt;
	ast_ifstmt ifstmt;
	ast_wstmt wstmt;
	ast_fstmt fstmt;
} ast_data;

typedef struct ast_node {
	ast_type type;
	ast_data ts_data; /* type specific data */
} ast_node;

#endif
