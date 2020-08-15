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

static
void
optimize_zeroloop(ilnode **root)
{
	_Bool trigger;
	ilnode *chld, *tmp;

	for (trigger = 0, chld = (*root)->chld; chld; chld = chld->next) {
		if (chld->type != ADD || chld->offset != (*root)->offset) {
			trigger = 0;
			break;
		/* NOTE: ideally the front end optimizes away zero adds but
		 *  we check here too just in case */
		} else if (chld->constant != 0) {
			trigger = 1;
		}
	}

	if (trigger) {
		for (chld = (*root)->chld; chld; ) {
			tmp = chld->next;
			free(chld);
			chld = tmp;
		}
		(*root)->type = MOV;
		(*root)->constant = 0;
		(*root)->chld = NULL;
	}
}

static
void
optimize_zeroloop_r(ilnode **root)
{
	ilnode **cur;

	for (cur = root; *cur; cur = &(*cur)->next)
		switch ((*cur)->type) {
		case WHILE:
			optimize_zeroloop(cur);
			optimize_zeroloop_r(&(*cur)->chld);
			break;
		default:
			break;
		}
}

static
void
optimize_merge(ilnode **root)
{
	ilnode **cur, *tmp;

	for (cur = &(*root)->next; *cur; )
		switch ((*cur)->type) {
		/* Candidates */
		case MOV:
			if ((*root)->offset == (*cur)->offset) {
				(*root)->constant = (*cur)->constant;
				tmp = (*cur)->next;
				free(*cur);
				*cur = tmp;
			} else {
				cur = &(*cur)->next;
			}
			break;
		case ADD:
			if ((*root)->offset == (*cur)->offset) {
				(*root)->constant += (*cur)->constant;
				tmp = (*cur)->next;
				free(*cur);
				*cur = tmp;
			} else {
				cur = &(*cur)->next;
			}
			break;
		case READ:
		case WRITE:
			/* Read/Write is only a barrier if touches us */
			if ((*root)->offset == (*cur)->offset)
				return;
			cur = &(*cur)->next;
			break;
		case WHILE:
			/* Do some redundant loop elimination here */
			if ((*root)->offset == (*cur)->offset &&
					!(*root)->constant) {
				free_iltree((*cur)->chld);
				tmp = (*cur)->next;
				free(*cur);
				*cur = tmp;
			} else {
				/* Loop is not redundant, thus a barrier */
				return;
			}
			break;
		/* Everything else is always considered barrier */
		default:
			return;
		}
}

static
void
optimzie_merge_r(ilnode **root)
{
	ilnode **cur;

	for (cur = root; *cur; cur = &(*cur)->next)
		switch ((*cur)->type) {
		case MOV:
			optimize_merge(cur);
			break;
		case WHILE:
			optimzie_merge_r(&(*cur)->chld);
			break;
		default:
			break;
		}
}

void
optimize_iltree(ilnode **root)
{
	optimize_zeroloop_r(root);
	optimzie_merge_r(root);
}

void
print_iltree(ilnode *root, int spaces)
{
	for (; root; root = root->next) {
		for (int i = 0; i < spaces; ++i)
			putchar(' ');

		switch (root->type) {
		case MOV:
			printf("MOV [BP + %d], %d\n",
				root->offset, root->constant);
			break;
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
