struct ast_node;

typedef enum ast_type {
	type_expr,
	type_constnum,
} ast_type;

typedef enum ast_expr_op {
	ast_add,
	ast_sub,
} ast_expr_op;

typedef struct ast_expr {
	struct ast_node *left;
	ast_expr_op op;
	struct ast_node *right;
} ast_expr;

typedef struct eprime_return {
	ast_expr_op op;
	struct ast_node *right;
} eprime_return;
	
typedef union ast_data {
	ast_expr expr;
	int constnum;
} ast_data;

typedef struct ast_node {
	ast_type type;
	ast_data ts_data; /*type specific data*/
} ast_node;