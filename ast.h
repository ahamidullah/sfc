struct ast_node;

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
	struct ast_node *left;
	ast_terminal op;
	struct ast_node *right;
} ast_expr;

typedef struct ast_astmt {
	struct ast_node *lval;
	struct ast_node *rval;
} ast_astmt;

typedef struct ast_ifstmt {
	struct ast_node *condexpr;
	struct ast_node *stmtlist;
} ast_ifstmt;

typedef struct ast_wstmt {
	struct ast_node *condexpr;
	struct ast_node *stmtlist;
} ast_wstmt;

typedef struct ast_fstmt {
	struct ast_node *init;
	struct ast_node *condexpr;
	struct ast_node *onloop;
	struct ast_node *stmtlist;
} ast_fstmt;

typedef struct ast_stmtlist {
	struct ast_node *stmt;
	struct ast_node *next;
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
	ast_data ts_data; /*type specific data*/
} ast_node;