/*
 * Common IL functions
 */
#include <stdio.h>
#include <stdlib.h>
#include "il.h"
#include "bitvec.h"

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
				/* If root was an add it still becomes a MOV */
				(*root)->type = MOV;
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
		default:
			/* Everything else is always considered barrier */
			return;
		}
}

static
void
optimize_merge_r(ilnode **root)
{
	ilnode **cur, *tmp;

	for (cur = root; *cur;)
		switch ((*cur)->type) {
		case MOV:
		case ADD:
			optimize_merge(cur);
			if ((*cur)->type == ADD && !(*cur)->constant) {
				tmp = (*cur)->next;
				free(*cur);
				*cur = tmp;
			} else {
				cur = &(*cur)->next;
			}
			break;
		case WHILE:
			optimize_merge_r(&(*cur)->chld);
			cur = &(*cur)->next;
			break;
		default:
			cur = &(*cur)->next;
			break;
		}
}

static
void
optimize_movify(ilnode **root)
{
	bitvec tainted;
	ilnode **cur, *tmp;

	tainted = BITVEC_INIT;

	for (cur = root; *cur; )
		switch ((*cur)->type) {
		case ADD:
			if (!bitvec_isset(&tainted, (*cur)->offset))
				(*cur)->type = MOV;
		case MOV:
		case READ:
			bitvec_set(&tainted, (*cur)->offset);
		case WRITE:
			cur = &(*cur)->next;
			break;
		case WHILE:
			if ((*root)->offset == (*cur)->offset &&
				!bitvec_isset(&tainted, (*cur)->offset)) {
				free_iltree((*cur)->chld);
				tmp = (*cur)->next;
				free(*cur);
				*cur = tmp;
				break;
			}
		default:
			return;
		}

	if (tainted.array)
		free(tainted.array);
}

static
void
duplicate(ilnode ***tgt, ilnode *tree, int cnt)
{
	ilnode *cur;

	while (cnt--)
		for (cur = tree; cur; cur = cur->next) {
			append_ilnode(*tgt,
				cur->type, cur->offset, cur->constant);
			*tgt = &(**tgt)->next;
		}
}

static
void
optimize_unroll(ilnode **root)
{
	ilnode **cur, *tmp, *tmp2;
	int cntmod;

	for (cur = &(*root)->next; *cur;)
		switch ((*cur)->type) {
		case MOV:
		case ADD:
		case READ:
			/* Can't find a dependent loop */
			if ((*cur)->offset == (*root)->offset)
				return;
		case WRITE: /* We can just ignore writes */
			cur = &(*cur)->next;
			break;
		case WHILE:
			/* Can't go further,
				found a loop dependent on something else */
			if ((*cur)->offset != (*root)->offset)
				return;

			/* See if the loop can be eliminated */
			if (!(*root)->constant) {
				free_iltree((*cur)->chld);
				tmp = (*cur)->next;
				free(*cur);
				*cur = tmp;
				break;
			}

			cntmod = 0;
			for (tmp = (*cur)->chld; tmp; tmp = tmp->next)
				switch (tmp->type) {
				case MOV:
					if (tmp->offset == (*root)->offset)
						cntmod = tmp->constant;
					break;
				case ADD:
					if (tmp->offset == (*root)->offset)
						cntmod += tmp->constant;
					break;
				/* Check for non-balanced or nested loop */
				case WHILE:
				case ADDBP:
					return;
				case READ:
					/* Loop counter comes from user */
					if (tmp->offset == (*root)->offset)
						return;
				case WRITE:
					break;
				}

			/* Infinite loop, not unrollable */
			if (!cntmod || (*root)->constant % cntmod)
				return;

			/* Found unroll candidate */
			if (cntmod < 0)
				cntmod = (*root)->constant / -cntmod;
			else
				cntmod = (255 - (*root)->constant) / cntmod;

			tmp = (*cur)->chld;
			tmp2 = (*cur)->next;
			free(*cur);
			duplicate(&cur, tmp, cntmod);
			free_iltree(tmp);
			*cur = tmp2;
			return;
		case ADDBP:
			/* We can't search further */
			return;
		}
}

static
void
optimize_unroll_r(ilnode **root)
{
	for (; *root; root = &(*root)->next)
		switch ((*root)->type) {
		case MOV:
			optimize_unroll(root);
			break;
		case WHILE:
			optimize_unroll_r(&(*root)->chld);
			break;
		default:
			break;
		}
}

void
optimize_iltree(ilnode **root)
{
	int i;
	optimize_zeroloop_r(root);
	for (i = 0; i < 3; ++i) {
		optimize_merge_r(root);
		optimize_movify(root);
		optimize_unroll_r(root);
		optimize_movify(root);
		optimize_merge_r(root);
	}
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
