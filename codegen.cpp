#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "ast.h"

typedef enum instruction_op {
	err_op = 0,
	mov_op,
	add_op,
	sub_op,
	imul_op,
	idiv_op,
} instruction_op;

typedef enum operand_type {
	register_rnd,
	memory_rnd,
	constant_rnd,
	none_rnd
} operand_type;

typedef struct operand {
	/* just use a string? */
	operand_type type;
	int value;
} operand;

typedef struct instruction {
	instruction_op op;
	operand o1;
	operand o2;
} instruction;

typedef struct operand_stack {
	operand data[1024]; //temp
	size_t size;
} operand_stack;

typedef struct instruction_queue {
	instruction *data[1024]; //temp
	size_t front;
	size_t size;
} instruction_queue;

typedef struct symbol {
	const char *name;
	int loc;
} symbol;

typedef struct symbol_table {
	symbol data[1024]; //temp
	size_t size;
} symbol_table;

symbol *
lookup(symbol_table *t, const char *name)
{
	for (int i = 0; i < t->size; ++i) {
		if (strcmp(name, t->data[i].name) == 1)
			return &t->data[i];
	}
	return NULL;
}

void
add_symbol(symbol_table *t, const char *name, int loc)
{
	t->data[t->size++].name = name;
	t->data[t->size].loc = loc;
}

static void
push_operand(operand target, operand_stack *stack)
{
	stack->data[stack->size++] = target;
}

static operand
pop_operand(operand_stack *stack)
{
	return stack->data[--stack->size];
}

static void
queue_instruction(instruction *instr, instruction_queue *q)
{
	q->data[q->size++] = instr;
	q->data[q->size] = NULL;
}

static instruction *
unqueue_instruction(instruction_queue *q)
{
	return q->data[q->front++];
}

static void
add_instruction(instruction_op op, operand o1, operand o2, instruction_queue *instrs)
{
	instruction *ni = (instruction *)malloc(sizeof(instruction));
	ni->op = op;
	ni->o1 = o1;
	ni->o2 = o2;
	queue_instruction(ni, instrs);
}

/* boilerplate */
static instruction_op
ast_op_to_instr_op(ast_terminal ast_op)
{
	switch(ast_op) {
		case ast_add:
			return add_op;
		case ast_sub:
			return sub_op;
		case ast_mult:
			return imul_op;
		case ast_div:
			return idiv_op;
	}
	return err_op;
}

static int
get_virt_reg()
{
	static int reg_count = 0;
	return reg_count++;
}

static void
traverse_ast(ast_node *node, operand_stack *targets, symbol *symtab, instruction_queue *instrs)
{
	switch(node->type) {
		case type_expr:
		{
			traverse_ast(node->ts_data.expr.left, targets, symtab, instrs);
			traverse_ast(node->ts_data.expr.right, targets, symtab, instrs);
			operand r_expr = pop_operand(targets);
			operand l_expr = pop_operand(targets);
			add_instruction(ast_op_to_instr_op(node->ts_data.expr.op), l_expr, r_expr, instrs);
			/* x86 stores result into first register */
			push_operand(l_expr, targets);
			return;
		}
		case type_astmt:
		{
			traverse_ast(node->ts_data.astmt.lval, targets, symtab, instrs);
			traverse_ast(node->ts_data.astmt.rval, targets, symtab, instrs);
			operand source = pop_operand(targets);
			operand target = pop_operand(targets);
			add_instruction(mov_op, target, source, instrs);
			push_operand(target, targets);
			/*gen_astmt(pop_operand(targets), pop_operand(targets), targets, instrs);*/
			return;
		}
		case type_name:
		{
			operand target_reg = { register_rnd, get_virt_reg() };
			/* just stick a one in there for now */
			operand constant = { constant_rnd, 1 };
			add_instruction(mov_op, target_reg, constant, instrs);
			push_operand(target_reg, targets);
			/*gen_name(node->ts_data.name, targets, instrs);*/
			return;
		}
		case type_stmtlist:
		{
			for (; node != END_STMT_LIST; node = node->ts_data.stmtlist.next) {
				traverse_ast(node->ts_data.stmtlist.stmt, targets, symtab, instrs);
			}
			return;
		}
		case type_num:
		{
			operand target_reg = { register_rnd, get_virt_reg() };
			operand constant = { constant_rnd, node->ts_data.num };
			add_instruction(mov_op, target_reg, constant, instrs);
			push_operand(target_reg, targets);
			/*gen_num(node->ts_data.num, targets, instrs);*/
			return;
		}
	}
}

static void
print_instructions(instruction_queue *q)
{
	auto print_operand = [](operand o) {
		printf("{ ");
		switch (o.type) {
			case register_rnd:
				printf("REG ");
				break;
			case constant_rnd:
				printf("CONST ");
				break;
			default:
				printf("??? ");
				break;
		}
		printf("%d } ", o.value);
	};
	for (instruction *cur = unqueue_instruction(q); cur; cur = unqueue_instruction(q)) {
		switch (cur->op) {
			case mov_op:
				printf("MOV ");
				break;
			case add_op:
				printf("ADD ");
				break;
			case sub_op:
				printf("SUB ");
				break;
			case imul_op:
				printf("MUL ");
				break;
			case idiv_op:
				printf("DIV ");
				break;
			default:
				printf("__ERROR__ ");
				break;
		}
		print_operand(cur->o1);
		print_operand(cur->o2);
		printf("\n");
	}
}

void
gencode(ast_node *ast)
{
	operand_stack targets;
	targets.size = 0;
	instruction_queue instrs;
	instrs.size = 0;
	instrs.front = 0;
	symbol_table symtab;
	traverse_ast(ast, &targets, &symtab, &instrs);
	print_instructions(&instrs);
	return;
}
