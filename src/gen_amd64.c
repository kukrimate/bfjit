/*
 * AMD64 code generator
 */
#include <stdio.h>
#include "il.h"

static const char *header =
"section .data\n"
"stack: resb 4096\n"
"stack_top:\n"
"tape: resb 4096\n"
"section .text\n"
"write:\n"
"movzx rax, al\n"
"push rax\n"
"mov rdi, 1\n"
"mov rsi, rsp\n"
"mov rdx, 1\n"
"mov rax, 1\n"
"syscall\n"
"pop rax\n"
"ret\n"
"global _start\n"
"_start:\n"
"mov rbp, tape\n"
"mov rsp, stack_top\n";

static const char *footer =
"xor rdi, rdi\n"
"mov rax, 60\n"
"syscall\n";

static
void
geninsn(ilnode *cur);

static
void
genloop(ilnode *root, int offset);

void
geninsn(ilnode *cur)
{
	switch (cur->type) {
	case ADD:
		printf("add byte [rbp%+d], %d\n", cur->offset, cur->constant);
		break;
	case ADDBP:
		printf("add rbp, %d\n", cur->offset);
		break;
	case READ:
		// TODO: implement input
		break;
	case WRITE:
		printf("mov al, [rbp%+d]\n", cur->offset);
		printf("call write\n");
		break;
	case WHILE:
		genloop(cur->chld, cur->offset);
		break;
	}
}

/*
 * NOTE: this is ugly but we are generating assembly, which means each
 * "loop" *must* have a globally unique identifier
 */
static int loopnum = 0;

void
genloop(ilnode *cur, int offset)
{
	int selfnum;

	selfnum = loopnum++;

	printf("start_%d:\n", selfnum);
	printf("cmp byte [rbp%+d], 0\n", offset);
	printf("jz end_%d\n", selfnum);

	for (; cur; cur = cur->next)
		geninsn(cur);

	printf("jmp start_%d\n", selfnum);
	printf("end_%d:\n", selfnum);
}

void
genasm(ilnode *root)
{
	printf("%s", header);

	for (; root; root = root->next)
		geninsn(root);

	printf("%s", footer);
}
