/*
 * AMD64 code generator
 */
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include "il.h"

#ifndef __x86_64__
#error This program only works on AMD64 compatible processors!
#endif

static uint8_t *codeptr;

static
void
geninsn(ilnode *cur);

static
void
genloop(ilnode *root);

void
geninsn(ilnode *cur)
{
	switch (cur->type) {
	case ADD:
		*codeptr++ = 0x80;
		*codeptr++ = 0x45;
		*codeptr++ = cur->offset;
		*codeptr++ = cur->constant;
			/* add byte [rbp+cur->offset], cur->constant */
		break;
	case ADDBP:
		*codeptr++ = 0x48;
		*codeptr++ = 0x83;
		*codeptr++ = 0xc5;
		*codeptr++ = cur->offset; /* add rbp, cur->offset */
		break;
	case READ:
		// TODO: implement input
		break;
	case WRITE:
		*codeptr++ = 0x0f;
		*codeptr++ = 0xb6;
		*codeptr++ = 0x7d;
		*codeptr++ = cur->offset;
			/* movzx edi, byte [rbp+cur->offset] */

		*codeptr++ = 0xff;
		*codeptr++ = 0xd3; /* call rbx (putchar) */
		break;
	case WHILE:
		genloop(cur);
		break;
	}
}

void
genloop(ilnode *root)
{
	uint8_t *startptr;
	size_t tmp;

	startptr = codeptr;
	*codeptr++ = 0x80;
	*codeptr++ = 0x7d;
	*codeptr++ = root->offset;
	*codeptr++ = 0; /* cmp byte [rbp+cur->offset], 0 */

	*codeptr++ = 0x0f;
	*codeptr++ = 0x84; /* jz endptr */
	codeptr += 4; /* leave room for offset */

	for (root = root->chld; root; root = root->next)
		geninsn(root);

	tmp = startptr - codeptr - 5;
	*codeptr++ = 0xe9;
	*codeptr++ = tmp & 0xff;
	*codeptr++ = tmp >> 8 & 0xff;
	*codeptr++ = tmp >> 16 & 0xff;
	*codeptr++ = tmp >> 24 & 0xff;

	tmp = codeptr - startptr - 10;
	startptr[6] = tmp & 0xff;
	startptr[7] = tmp >> 8 & 0xff;
	startptr[8] = tmp >> 16 & 0xff;
	startptr[9] = tmp >> 24 & 0xff;
}

/*
 * Amount of memory to map for code generation
 */
#define CODELEN 16384

void
gencode(ilnode *root)
{
	uint8_t *code;

	codeptr = code = mmap(NULL, CODELEN, PROT_READ | PROT_WRITE | PROT_EXEC,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	*codeptr++ = 0x55; /* push rbp */
	*codeptr++ = 0x53; /* push rbx */

	*codeptr++ = 0x48;
	*codeptr++ = 0x81;
	*codeptr++ = 0xec;
	*codeptr++ = 0x00;
	*codeptr++ = 0x20;
	*codeptr++ = 0x00;
	*codeptr++ = 0x00; /* sub rsp, 0x2000 */

	/* memset the variable storage to 0 */
	*codeptr++ = 0x48;
	*codeptr++ = 0x89;
	*codeptr++ = 0xe7; /* mov rdi, rsp */

	*codeptr++ = 0xb9;
	*codeptr++ = 0x00;
	*codeptr++ = 0x20;
	*codeptr++ = 0x00;
	*codeptr++ = 0x00; /* mov ecx, 0x2000 */

	*codeptr++ = 0x31;
	*codeptr++ = 0xc0; /* xor eax, eax */

	*codeptr++ = 0xf3;
	*codeptr++ = 0xaa; /* rep stosb */

	/* setup pointers */
	*codeptr++ = 0x48;
	*codeptr++ = 0x89;
	*codeptr++ = 0xe5; /* mov rbp, rsp */

	*codeptr++ = 0x48;
	*codeptr++ = 0xbb;
	*codeptr++ = (uintptr_t) putchar & 0xff;
	*codeptr++ = (uintptr_t) putchar >> 8 & 0xff;
	*codeptr++ = (uintptr_t) putchar >> 16 & 0xff;
	*codeptr++ = (uintptr_t) putchar >> 24 & 0xff;
	*codeptr++ = (uintptr_t) putchar >> 32 & 0xff;
	*codeptr++ = (uintptr_t) putchar >> 40 & 0xff;
	*codeptr++ = (uintptr_t) putchar >> 48 & 0xff;
	*codeptr++ = (uintptr_t) putchar >> 56 & 0xff; /* mov rbx, putchar */

	for (; root; root = root->next)
		geninsn(root);

	*codeptr++ = 0x48;
	*codeptr++ = 0x81;
	*codeptr++ = 0xc4;
	*codeptr++ = 0x00;
	*codeptr++ = 0x20;
	*codeptr++ = 0x00;
	*codeptr++ = 0x00; /* add rsp, 0x2000 */

	*codeptr++ = 0x5b; /* pop rbx */
	*codeptr++ = 0x5d; /* pop rbp */
	*codeptr++ = 0xC3; /* ret */

	int (*ret)() = (int(*)())code;
	ret();

	munmap(code, CODELEN);
}
