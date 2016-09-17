#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <functional>
#include "ast.h"

enum operand_type {
	register_rnd,
	memory_rnd,
	constant_rnd,
	none_rnd
};

struct operand {
	/* just use a string? */
	operand_type type;
	int value;
};

struct instruction {
	const char *op;
	operand o1;
	operand o2;
};

struct operand_stack {
	operand data[1024]; //temp
	size_t size;
};

struct instruction_queue {
	instruction *data[1024]; //temp
	size_t front;
	size_t size;
};

struct symbol {
	const char *name;
	int loc;
};

struct symbol_table {
	symbol data[1024]; //temp
	size_t size;
};

static
symbol *
lookup(const char *name, symbol_table *t)
{
	for (size_t i = 0; i < t->size; ++i) {
		if (strcmp(name, t->data[i].name) == 0)
			return &t->data[i];
	}
	return NULL;
}

static
void
add_symbol(symbol_table *t, const char *name, int loc)
{
	t->data[t->size].name = name;
	t->data[t->size].loc = loc;
	++t->size;
}

static
void
push_operand(operand target, operand_stack *stack)
{
	stack->data[stack->size++] = target;
}

static
operand
pop_operand(operand_stack *stack)
{
	return stack->data[--stack->size];
}

static
void
queue_instruction(instruction *instr, instruction_queue *q)
{
	q->data[q->size++] = instr;
	q->data[q->size] = NULL;
}

static
instruction *
unqueue_instruction(instruction_queue *q)
{
	return q->data[q->front++];
}

static
void
add_instruction(const char *op, operand o1, operand o2, instruction_queue *instrs)
{
	instruction *ni = (instruction *)malloc(sizeof(instruction));
	ni->op = op;
	ni->o1 = o1;
	ni->o2 = o2;
	queue_instruction(ni, instrs);
}

static
int
get_virt_reg()
{
	static int reg_count = 0;
	return reg_count++;
}

static
operand_stack *
make_stack()
{
	operand_stack *os = (operand_stack *)malloc(sizeof(operand_stack));
	os->size = 0;
	return os;
}

static
instruction_queue *
make_queue()
{
	instruction_queue *iq = (instruction_queue *)malloc(sizeof(instruction_queue));
	iq->size = 0;
	iq->front = 0;
	return iq;
}

static
symbol_table *
make_symtab()
{
	symbol_table *st = (symbol_table *)malloc(sizeof(symbol_table));
	return st;
}

static
instruction_queue *
gen_virt_instrs(ast_node *ast)
{
	operand_stack      *targets  =  make_stack();
	symbol_table       *symtab   =  make_symtab();
	instruction_queue  *instrs   =  make_queue();

	// c++ lambda nonesense. it ain't great...
	std::function<void(ast_node *)> traverse_ast = [targets, symtab, instrs, &traverse_ast](ast_node *n) {
		switch(n->type) {
			case type_expr:
			{
				traverse_ast(n->expr.left);
				traverse_ast(n->expr.right);
				operand r_expr = pop_operand(targets);
				operand l_expr = pop_operand(targets);
				add_instruction(n->expr.op, l_expr, r_expr, instrs);
				/* x86 stores result into first register */
				push_operand(l_expr, targets);
				return;
			}
			case type_astmt:
			{
				traverse_ast(n->astmt.lval);
				traverse_ast(n->astmt.rval);
				operand source = pop_operand(targets);
				operand target = pop_operand(targets);
				add_instruction("MOV", target, source, instrs);
				push_operand(target, targets);
				return;
			}
			case type_name:
			{
				symbol *s = lookup(n->name, symtab);
				int target_reg = 0;
				if (s) {
					target_reg = s->loc;
					printf("%s %d\n", s->name, s->loc);
				} else {
					target_reg = get_virt_reg();
					add_symbol(symtab, n->name, target_reg);
				}
				push_operand({ register_rnd, target_reg }, targets);
				return;
			}
			case type_stmtlist:
			{
				for (; n != END_STMT_LIST; n = n->stmtlist.next)
					traverse_ast(n->stmtlist.stmt);
				return;
			}
			case type_num:
			{
				operand target_reg = { register_rnd, get_virt_reg() };
				operand constant = { constant_rnd, n->num };
				add_instruction("MOV", target_reg, constant, instrs);
				push_operand(target_reg, targets);
				return;
			}
		}
	};

	traverse_ast(ast);
	free(targets);
	free(symtab);
	return instrs;
}

static
void
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
	for (const instruction *cur = unqueue_instruction(q); cur; cur = unqueue_instruction(q)) {
		printf("%s ", cur->op);
		print_operand(cur->o1);
		print_operand(cur->o2);
		printf("\n");
	}
}

void
gencode(ast_node *ast)
{
	instruction_queue *instrs = gen_virt_instrs(ast);
	print_instructions(instrs);
	free(instrs);
	return;
}

