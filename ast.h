struct ast_node;

typedef enum ast_type {
	type_expr,
	type_name,
	type_num,
} ast_type;

typedef enum ast_terminal {
	ast_add,
	ast_sub,
	ast_mult,
	ast_div,
	ast_paren_open,
	ast_paren_close,
} ast_terminal;

typedef struct ast_expr {
	struct ast_node *left;
	ast_terminal op;
	struct ast_node *right;
} ast_expr;

typedef union ast_data {
	ast_expr expr;
	char *name;
	int num;
} ast_data;

typedef struct ast_node {
	ast_type type;
	ast_data ts_data; /*type specific data*/
} ast_node;