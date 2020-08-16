/*
 * Brainfuck frontend and driver
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "il.h"

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
gencode(ilnode *root);

int
main(int argc, char *argv[])
{
	int opt, flag_i, flag_o;
	FILE *fp;
	ilnode *iroot;

	flag_i = flag_o = 0;
	while ((opt = getopt(argc, argv, "io")) > 0)
		switch (opt) {
		case 'i':
			flag_i = 1;
			break;
		case 'o':
			flag_o = 1;
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
	if (flag_o)
		optimize_iltree(&iroot);
	if (flag_i)
		print_iltree(iroot, 0);
	else
		gencode(iroot);

	free_iltree(iroot);
	fclose(fp);
	return 0;

print_usage:

	fprintf(stderr, "Usage: %s [-i] [-o] PROG\n", argv[0]);
	return 1;
}
