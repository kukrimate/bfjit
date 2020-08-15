#ifndef IL_H
#define IL_H

/*
 * Width of one level of indentation in the debug output
 */
#define INDENT_WIDTH 4

/*
 * Intermediate language
 */

/* Operation type */
typedef enum {
	MOV,
	ADD,
	ADDBP,
	READ,
	WRITE,
	WHILE
} iltype;

/* Tree node */
typedef struct ilnode ilnode;

struct ilnode {
	iltype	type;
	int	offset;
	int	constant;
	ilnode	*chld;
	ilnode	*next;
};

void
append_ilnode(ilnode **tail, iltype type, int offset, int constant);

void
optimize_iltree(ilnode **root);

void
print_iltree(ilnode *root, int spaces);

void
free_iltree(ilnode *root);

#endif
