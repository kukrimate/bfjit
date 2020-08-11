#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * Width of one level of indentation in the debug output
 */
#define INDENT_WIDTH 4

/*
 * Intermediate language
 */

/* Operation type */
typedef enum {
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
build_iltree(ilnode **tail, FILE *fp, int startoffs, _Bool subtree)
{
	int ch, offset, constant;

	offset = 0;
	while ((ch = fgetc(fp)) != EOF) {
		switch (ch) {
		case '<':
			--offset;
			break;
		case '>':
			++offset;
			break;
		case '+':
		case '-':
			for (constant = 0;; ch = fgetc(fp))
				switch (ch) {
				case '+':
					++constant;
					break;
				case '-':
					--constant;
					break;
				default:
					ungetc(ch, fp);
					goto breakloop;
				}
breakloop:
			if (constant) {
				append_ilnode(tail, ADD,
					startoffs + offset, constant);
				tail = &(*tail)->next;
			}
			break;
		case '.':
			append_ilnode(tail, WRITE, startoffs + offset, 0);
			tail = &(*tail)->next;
			break;
		case ',':
			append_ilnode(tail, READ, startoffs + offset, 0);
			tail = &(*tail)->next;
			break;
		case '[':
			append_ilnode(tail, WHILE, startoffs + offset, 0);
			build_iltree(&(*tail)->chld, fp, startoffs + offset, 1);
			tail = &(*tail)->next;
			break;
		case ']':
			goto endfunc;
		}
	}
endfunc:
	if (subtree && offset)
		append_ilnode(tail, ADDBP, offset, 0);
}

void
print_iltree(ilnode *root, int spaces)
{
	for (; root; root = root->next) {
		for (int i = 0; i < spaces; ++i)
			putchar(' ');

		switch (root->type) {
		case ADD:
			printf("ADD [%d], %d\n", root->offset, root->constant);
			break;
		case ADDBP:
			printf("ADDBP %d\n", root->offset);
			break;
		case READ:
			printf("READ [%d]\n", root->offset);
			break;
		case WRITE:
			printf("WRITE [%d]\n", root->offset);
			break;
		case WHILE:
			printf("WHILE [%d]\n", root->offset);
			print_iltree(root->chld, spaces + INDENT_WIDTH);
			break;
		}
	}
}

void
genasm(ilnode *root, int offset);

int
main(int argc, char *argv[])
{
	int opt, flag_i;
	FILE *fp;
	ilnode *iroot;

	flag_i = 0;
	while ((opt = getopt(argc, argv, "i")) > 0)
		switch (opt) {
		case 'i':
			flag_i = 1;
			break;
		default:
			goto print_usage;
		}

	if (optind >= argc) {
		goto print_usage;
	}

	if (!(fp = fopen(argv[optind], "r"))) {
		perror(argv[optind]);
		return 1;
	}

	build_iltree(&iroot, fp, 0, 0);
	if (flag_i)
		print_iltree(iroot, 0);

	genasm(iroot, 0);

	fclose(fp);
	return 0;

print_usage:

	fprintf(stderr, "Usage: %s [-i] PROG\n", argv[0]);
	return 1;
}
