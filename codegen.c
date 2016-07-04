#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "ast.h"

typedef struct operand_stack {
	operand data[1024];
	size_t size;
} operand_stack;

typedef struct instruction_stack {
	instruction *data[1024];
	size_t size;
} instruction_stack;

static void
push_operand(operand_stack *stack, operand target)
{
	stack->data[stack->size++] = target;
}

static operand
pop_operand(operand_stack *stack)
{
	return stack->data[--stack->size];
}

static void
push_instruction(instruction_stack *stack, instruction *instr)
{
	stack->data[stack->size++] = instr;
}

static instruction *
pop_instruction(instruction_stack *stack)
{
	return stack->data[--stack->size];
}

static void
make_instruction(instruction_op op, operand o1, operand o2, operand target, operand_stack *targets, instruction_stack *instrs)
{
	instruction *new_instr = malloc(sizeof(instruction));
	new_instr->op = op;
	new_instr->o1 = o1;
	new_instr->o2 = o2;
	new_instr->next = NULL;
	if (targets)
		push_operand(targets, target);
	if (instrs)
		push_instruction(instrs, new_instr);	
}

/*boilerplate*/
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
	return -1;
}

static void
gen_astmt(operand lval, operand rval, operand_stack *targets, instruction_stack *instrs)
{
	make_instruction(mov_op, lval, rval, lval, NULL, instrs);
}

static void
gen_expr(operand o1, operand o2, ast_terminal op, operand_stack *targets, instruction_stack *instrs)
{
	make_instruction(ast_op_to_instr_op(op), o1, o2, o2, targets, instrs);
}

static void
gen_asmt(operand source, operand target, operand_stack *targets, instruction_stack *instrs)
{
	make_instruction(mov_op, target, source, target, targets, instrs);
}

static unsigned int
get_virt_reg()
{
	static unsigned int reg_count = 0;
	return reg_count++;
}

static void
gen_name(char *name, operand_stack *targets, instruction_stack *instrs)
{
	operand target_reg = { register_rnd, get_virt_reg() };
	operand constant = { constant_rnd, 1 };
	/*just stick a one in there for now*/
	make_instruction(mov_op, target_reg, constant, target_reg, targets, instrs);
}

static void
gen_num(int num, operand_stack *targets, instruction_stack *instrs)
{
	operand target_reg = { register_rnd, get_virt_reg() };
	operand constant = { constant_rnd, num };
	make_instruction(mov_op, target_reg, constant, target_reg, targets, instrs);
}

static void
traverse_ast(ast_node *node, operand_stack *targets, instruction_stack *instrs)
{
	switch(node->type) {
		case type_expr:
			traverse_ast(node->ts_data.expr.left, targets, instrs);
			traverse_ast(node->ts_data.expr.right, targets, instrs);
			gen_expr(pop_operand(targets), pop_operand(targets), node->ts_data.expr.op, targets, instrs);
			return;
		case type_astmt:
			traverse_ast(node->ts_data.astmt.lval, targets, instrs);
			traverse_ast(node->ts_data.astmt.rval, targets, instrs);
			gen_astmt(pop_operand(targets), pop_operand(targets), targets, instrs);
			return;
		case type_name:
			gen_name(node->ts_data.name, targets, instrs);
			return;
		case type_stmtlist:
			for (; node != END_STMT_LIST; node = node->ts_data.stmtlist.next) {
				traverse_ast(node->ts_data.stmtlist.stmt, targets, instrs);
			}
			return;
		case type_num:
			gen_num(node->ts_data.num, targets, instrs);
			return;
	}
}

static void
print_instrs()
{

}

void
codegen(ast_node *ast)
{
	operand_stack targets;
	targets.size = 0;
	instruction_stack instrs;
	instrs.size = 0;
	traverse_ast(ast, &targets, &instrs);	
}
