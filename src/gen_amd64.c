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

static int loopnum = 0;

void
genasm(ilnode *root, int offset)
{
	int selfnum;

	selfnum = loopnum++;
	if (selfnum) {
		printf("start_%d:\n", selfnum);
		printf("cmp byte [rbp%+d], 0\n", offset);
		printf("jz end_%d\n", selfnum);
	} else {
		printf("%s", header);
	}

	for (; root; root = root->next)
		switch (root->type) {
		case ADD:
			printf("add byte [rbp%+d], %d\n",
				root->offset, root->constant);
			break;
		case ADDBP:
			printf("add rbp, %d\n", root->offset);
			break;
		case READ:
			// TODO: implement input
			break;
		case WRITE:
			printf("mov al, [rbp%+d]\n", root->offset);
			printf("call write\n");
			break;
		case WHILE:
			genasm(root->chld, root->offset);
			break;
		}

	if (selfnum) {
		printf("jmp start_%d\n", selfnum);
		printf("end_%d:\n", selfnum);
	} else {
		printf("%s", footer);
	}
}
