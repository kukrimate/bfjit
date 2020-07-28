#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

typedef struct bfnode bfnode;

struct bfnode {
	char	op;	/* operation */
	bfnode	*chld;	/* children */
	bfnode	*next;	/* next in sequence */
};

void
append_bfnode(bfnode **tail, char op)
{
	(*tail) = malloc(sizeof(bfnode));
	(*tail)->op = op;
	(*tail)->chld = NULL;
	(*tail)->next = NULL;
}

void
build_bftree(bfnode **tail, FILE *fp)
{
	int ch;

	while ((ch = fgetc(fp)) != EOF)
		switch (ch) {
		case '<':
		case '>':
		case '+':
		case '-':
		case '.':
		case ',':
			append_bfnode(tail, ch);
			tail = &(*tail)->next;
			break;
		case '[':
			append_bfnode(tail, ch);
			build_bftree(&(*tail)->chld, fp);
			tail = &(*tail)->next;
			break;
		case ']':
			return;
		}
}

void
print_bftree(bfnode *root, int spaces)
{
	for (; root; root = root->next) {
		for (int i = 0; i < spaces; ++i)
			putchar(' ');
		printf("%c\n", root->op);
		print_bftree(root->chld, spaces + 2);
	}
}

typedef enum {
	ADD,
	ADDBP,
	READ,
	WRITE,
	WHILE
} iltype;

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

int
sum_bfnodes(bfnode **root)
{
	int s;

	for (s = 0; *root; *root = (*root)->next)
		switch ((*root)->op) {
		case '+':
			++s;
			break;
		case '-':
			--s;
			break;
		default:
			return s;
		}
	return s;
}

void
build_iltree(ilnode **tail, bfnode *root, int startoffset, _Bool subtree)
{
	int offset;

	for (offset = 0; root; ) {
		switch (root->op) {
		case '<':
			--offset;
			break;
		case '>':
			++offset;
			break;
		case '+':
		case '-':
			append_ilnode(tail,
				ADD, startoffset + offset, sum_bfnodes(&root));
			tail = &(*tail)->next;
			continue;
		case '.':
			append_ilnode(tail, WRITE, startoffset + offset, 0);
			tail = &(*tail)->next;
			break;
		case ',':
			append_ilnode(tail, READ, startoffset + offset, 0);
			tail = &(*tail)->next;
			break;
		case '[':
			append_ilnode(tail, WHILE, startoffset + offset, 0);
			build_iltree(&(*tail)->chld,
				root->chld, startoffset + offset, 1);
			tail = &(*tail)->next;
			break;
		}
		root = root->next;
	}
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
			print_iltree(root->chld, spaces + 2);
			break;
		}
	}
}

int
main(int argc, char *argv[])
{
	int opt, flag_b, flag_i;
	FILE *fp;
	bfnode *broot;
	ilnode *iroot;

	unsigned char *prog, *tape;

	flag_b = flag_i = 0;
	while ((opt = getopt(argc, argv, "bi")) > 0)
		switch (opt) {
		case 'b':
			flag_b = 1;
			break;
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

	build_bftree(&broot, fp);
	if (flag_b)
		print_bftree(broot, 0);

	build_iltree(&iroot, broot, 0, 0);
	if (flag_i)
		print_iltree(iroot, 0);

	prog = mmap(NULL, 0x10000, PROT_READ | PROT_WRITE | PROT_EXEC,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	tape = mmap(NULL, 0x10000, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	fclose(fp);
	return 0;

print_usage:

	fprintf(stderr, "Usage: %s [-bi] PROG\n", argv[0]);
	return 1;
}
