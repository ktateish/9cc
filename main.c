// vim:set filetype=c tabstop=8 shiftwidth=8 noexpandtab:
#include <stdio.h>
#include <string.h>

#include "9cc.h"

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <number>\n", argv[0]);
		fprintf(stderr, "Usage: %s -test\n", argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "-test") == 0) {
		runtest();
		return 0;
	}

	// tokenize
	init_tokens();
	tokenize(argv[1]);

	// Parse tokens
	program();

	// output first part assembry
	printf(".intel_syntax noprefix\n");
	printf(".global main\n");
	printf("main:\n");

	// Generate code
	for (int i = 0; code(i) != NULL; i++) {
		gen(code(i));
		printf("  pop rax\n");
	}

	// The value of the expression will be on the top of the stack
	// load it to rax and let it be return value.
	printf("  ret\n");

	return 0;
}
