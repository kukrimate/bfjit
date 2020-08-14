/*
 * AMD64 code generator
 */
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include "il.h"

#include <stdlib.h>

#ifndef __x86_64__
#error This program only works on AMD64 compatible processors!
#endif

static
void
write32(uint8_t **ptr, uint32_t val)
{
	*(*ptr)++ = val & 0xff;
	*(*ptr)++ = val >> 8 & 0xff;
	*(*ptr)++ = val >> 16 & 0xff;
	*(*ptr)++ = val >> 24 & 0xff;
}

static
void
write64(uint8_t **ptr, uint64_t val)
{
	*(*ptr)++ = val & 0xff;
	*(*ptr)++ = val >> 8 & 0xff;
	*(*ptr)++ = val >> 16 & 0xff;
	*(*ptr)++ = val >> 24 & 0xff;
	*(*ptr)++ = val >> 32 & 0xff;
	*(*ptr)++ = val >> 40 & 0xff;
	*(*ptr)++ = val >> 48 & 0xff;
	*(*ptr)++ = val >> 56 & 0xff;
}

/*
 * Allow variable length encoding
 */
#define ALLOW_VARIABLE 1

static
void
encode_add(uint8_t **ptr, int offset, int constant)
{
	*(*ptr)++ = 0x80;
	if (ALLOW_VARIABLE && INT8_MIN < offset && offset < INT8_MAX) {
		*(*ptr)++ = 0x45;
		*(*ptr)++ = offset;
	} else {
		*(*ptr)++ = 0x85;
		write32(ptr, offset);
	}
	*(*ptr)++ = constant;
}

static
void
encode_addbp(uint8_t **ptr, int offset)
{
	*(*ptr)++ = 0x48;
	if (ALLOW_VARIABLE && INT8_MIN < offset && offset < INT8_MAX) {
		*(*ptr)++ = 0x83;
		*(*ptr)++ = 0xc5;
		*(*ptr)++ = offset;
	} else {
		*(*ptr)++ = 0x81;
		*(*ptr)++ = 0xc5;
		write32(ptr, offset);
	}
}

static
void
encode_movzx(uint8_t **ptr, int offset)
{
	*(*ptr)++ = 0x0f;
	*(*ptr)++ = 0xb6;
	if (ALLOW_VARIABLE && INT8_MIN < offset && offset < INT8_MAX) {
		*(*ptr)++ = 0x7d;
		*(*ptr)++ = offset;
	} else {
		*(*ptr)++ = 0xbd;
		write32(ptr, offset);
	}
}

static
void
encode_cmp(uint8_t **ptr, int offset)
{
	*(*ptr)++ = 0x80;
	if (ALLOW_VARIABLE && INT8_MIN < offset && offset < INT8_MAX) {
		*(*ptr)++ = 0x7d;
		*(*ptr)++ = offset;
		*(*ptr)++ = 0;
	} else {
		*(*ptr)++ = 0xbd;
		write32(ptr, offset);
		*(*ptr)++ = 0;
	}
}

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
		encode_add(&codeptr, cur->offset, cur->constant);
			/* add byte [rbp+cur->offset], cur->constant */
		break;
	case ADDBP:
		encode_addbp(&codeptr, cur->offset); /* add rbp, cur->offset */
		break;
	case READ:
		// TODO: implement input
		break;
	case WRITE:
		encode_movzx(&codeptr, cur->offset);
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
	uint8_t *startptr, *offptr;

	startptr = codeptr;
	encode_cmp(&codeptr, root->offset);
		/* cmp byte [rbp + cur->offset], 0 */

	*codeptr++ = 0x0f;
	*codeptr++ = 0x84; /* jz endptr */

	offptr = codeptr;
	codeptr += 4; /* leave room for offset */

	for (root = root->chld; root; root = root->next)
		geninsn(root);

	*codeptr++ = 0xe9;
	write32(&codeptr, startptr - (codeptr + 4)); /* jmp to start */

	write32(&offptr, codeptr - (offptr + 4)); /* fill jz offset */
}

/*
 * Amount of memory to map for code generation
 */
#define CODELEN 32768

/*
 * Tape length
 */
#define TAPELEN 32768

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
	write32(&codeptr, TAPELEN); /* sub rsp, TAPELEN */

	/* memset the variable storage to 0 */
	*codeptr++ = 0x48;
	*codeptr++ = 0x89;
	*codeptr++ = 0xe7; /* mov rdi, rsp */

	*codeptr++ = 0xb9;
	write32(&codeptr, TAPELEN); /* mov ecx, TAPELEN */

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
	write64(&codeptr, (uintptr_t) putchar); /* mov rbx, putchar */

	for (; root; root = root->next)
		geninsn(root);

	*codeptr++ = 0x48;
	*codeptr++ = 0x81;
	*codeptr++ = 0xc4;
	write32(&codeptr, TAPELEN); /* add rsp, TAPELEN */

	*codeptr++ = 0x5b; /* pop rbx */
	*codeptr++ = 0x5d; /* pop rbp */
	*codeptr++ = 0xC3; /* ret */

	/*fwrite(code, codeptr - code, 1, stdout);*/
	int (*ret)() = (int(*)())code;
	ret();

	munmap(code, CODELEN);
}
