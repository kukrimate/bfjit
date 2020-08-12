/*
 * Common IL functions
 */
#include <stdio.h>
#include <stdlib.h>
#include "il.h"

void
append_ilnode(ilnode **tail, iltype type, int offset, int constant)
{
	(*tail) = malloc(sizeof(ilnode));
	(*tail)->type = type;
	(*tail)->offset = offset;
	(*tail)->constant = constant;
	(*tail)->chld = NULL;
	(*tail)->next = NULL;
}

void
print_iltree(ilnode *root, int spaces)
{
	for (; root; root = root->next) {
		for (int i = 0; i < spaces; ++i)
			putchar(' ');

		switch (root->type) {
		case ADD:
			printf("ADD [BP + %d], %d\n",
				root->offset, root->constant);
			break;
		case ADDBP:
			printf("ADDBP %d\n", root->offset);
			break;
		case READ:
			printf("READ [BP + %d]\n", root->offset);
			break;
		case WRITE:
			printf("WRITE [BP + %d]\n", root->offset);
			break;
		case WHILE:
			printf("WHILE [BP + %d]\n", root->offset);
			print_iltree(root->chld, spaces + INDENT_WIDTH);
			break;
		}
	}
}

void
free_iltree(ilnode *root)
{
	ilnode *tmp;

	while (root) {
		free_iltree(root->chld);
		tmp = root->next;
		free(root);
		root = tmp;
	}
}
