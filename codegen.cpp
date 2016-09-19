#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <functional>
#include "ast.h"

#define printerr(...)\
{\
	fprintf(stderr, "\n");\
	fprintf(stderr, "Error on line %d: ", __LINE__);\
	fprintf(stderr, __VA_ARGS__);\
	fprintf(stderr, "\n");\
}

#define abortc(...)\
{\
	printerr(__VA_ARGS__);\
	fprintf(stderr, "Aborting...\n\n");\
	exit(1);\
}

enum register_id {
	eax_reg = 0, //general purpose regs
	ebx_reg,
	ecx_reg,
	edx_reg,
	num_gen_regs,
	esp_reg, //special purpose regs
};

struct memory_table {
	int is_reg_in_use[num_gen_regs];
	size_t esp_offset;
};

static
memory_table *
make_memtab()
{
	memory_table *mt = (memory_table *)malloc(sizeof(memory_table));
	memset(mt->is_reg_in_use, 0, num_gen_regs);
	mt->esp_offset = 0;
	return mt;
}

enum operand_type {
	register_rnd = 0,
	memory_rnd,
	constant_rnd,
	none_rnd
};

struct operand {
	operand_type type;
	union {
		register_id reg;
		size_t offset;
		int constant;
	};
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
	size_t offset;
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
add_symbol(symbol_table *t, const char *name, int offset)
{
	t->data[t->size].name = name;
	t->data[t->size].offset = offset;
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
register_id
get_reg(memory_table *mt)
{
	for (int i = 0; i < num_gen_regs; ++i) {
		if (mt->is_reg_in_use[i] == 0) {
			mt->is_reg_in_use[i] = 1;
			return (register_id)i; //cast i to corresponding reg enum entry. kinda nasty
		}
	}
	abortc("Ran out of registers. That shouldn't happen...");
}

static
void
reset_regs(memory_table *mt)
{
	for (int i = 0; i < num_gen_regs; ++i)
		mt->is_reg_in_use[i] = 0;
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
void
add_instr_and_reset_regs(const char *op, operand o1, operand o2, instruction_queue *instrs, memory_table *mt)
{
	add_instruction(op, o1, o2, instrs);
	reset_regs(mt);
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

//awful. hacky. c++ doesn't let you initialize union by field, need a better way (constructors?)
static
operand
make_operand(operand_type type, int val)
{
	operand o;
	o.type = type;
	if (type != constant_rnd && type != none_rnd)
		abortc("operand mismatch");
	o.constant = val;
	return o;
}

static
operand
make_operand(operand_type type, register_id val)
{
	operand o;
	o.type = type;
	if (type != register_rnd)
		abortc("operand mismatch");
	o.reg = val;
	return o;
}

static
operand
make_operand(operand_type type, size_t val)
{
	operand o;
	o.type = type;
	if (type != memory_rnd)
		abortc("operand mismatch");
	o.offset = val;
	return o;
}

static operand NONE_OPERAND = make_operand(none_rnd, 0); //for operations that accept fewer operands

static
instruction_queue *
gen_asm(ast_node *ast)
{
	operand_stack      *targets  =  make_stack();
	symbol_table       *symtab   =  make_symtab();
	instruction_queue  *instrs   =  make_queue();
	memory_table       *memtab   =  make_memtab();

	// c++ lambda nonsense. it ain't great...
	std::function<void(ast_node *)> traverse_ast = [targets, symtab, instrs, memtab, &traverse_ast](ast_node *n) {
		switch (n->type) {
			case type_expr:
			{
				traverse_ast(n->expr.left);
				traverse_ast(n->expr.right);
				operand r_expr = pop_operand(targets);
				operand l_expr = pop_operand(targets);
				add_instr_and_reset_regs(n->expr.op, l_expr, r_expr, instrs, memtab);
				//x86 stores result into first register
				push_operand(l_expr, targets);
				return;
			}
			case type_astmt:
			{
				traverse_ast(n->astmt.lval);
				traverse_ast(n->astmt.rval);
				operand expr = pop_operand(targets);
				operand var = pop_operand(targets);
				//todo: check if new var and push it directly rather than pushing a zero
				add_instr_and_reset_regs("movl", expr, var, instrs, memtab);
				//push_operand(var, targets);
				return;
			}
			case type_exprvar: //var in an expr, load into a register (target)
			{
				symbol *s = lookup(n->name, symtab);
				if (!s)
					abortc("%s undefined", s->name);
				operand reg = make_operand(register_rnd, get_reg(memtab));
				add_instruction("movl", make_operand(memory_rnd, s->offset), reg, instrs);
				push_operand(reg, targets);
				return;
			}
			case type_asmtvar: //var being assigned to, memory is target
			{
				symbol *s = lookup(n->name, symtab);
				operand mem;
				if (s)
					mem = make_operand(memory_rnd, s->offset);
				else { //new var
					//just to make space
					add_instruction("pushl", make_operand(constant_rnd, 0), NONE_OPERAND, instrs);
					memtab->esp_offset += 4;
					mem = make_operand(memory_rnd, memtab->esp_offset);
					add_symbol(symtab, n->name, mem.offset);
				}
				push_operand(mem, targets);
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
				operand reg = make_operand(register_rnd, get_reg(memtab));
				operand val = make_operand(constant_rnd, n->num);
				add_instruction("movl", val, reg, instrs);
				push_operand(reg, targets);
				return;
			}
		}
	};

	traverse_ast(ast);
	free(targets);
	free(symtab);
	free(memtab);
	return instrs;
}

static
void
print_instructions(instruction_queue *q)
{
	auto print_operand = [](operand o) {
		switch (o.type) {
			case register_rnd:
				printf("{ REG ");
				printf("%d } ", (int)o.reg);
				break;
			case constant_rnd:
				printf("{ CONST ");
				printf("%d } ", o.constant);
				break;
			case memory_rnd:
				printf("{ MEM ");
				printf("%lu } ", o.offset);
				break;
			case none_rnd:
				break;
			default:
				printf("??? ");
				break;
		}
	};
	for (const instruction *cur = unqueue_instruction(q); cur; cur = unqueue_instruction(q)) {
		printf("%s ", cur->op);
		print_operand(cur->o1);
		print_operand(cur->o2);
		printf("\n");
	}
}

void
gen_code(ast_node *ast)
{
	instruction_queue *instrs = gen_asm(ast);
	print_instructions(instrs);
	free(instrs);
	return;
}

