#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

// token types
enum {
	TK_NUM = 256, // integer
	TK_EOF,
};

// token
typedef struct {
	int  ty;
	int val;
	char *input;
} Token;

// input
char *user_input;

// tokenized results
// assume at most 100 tokens
Token tokens[100];

// function for reporting an error
void error(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

// function for reporting error location
void error_at(char *loc, char *msg) {
	int pos = loc - user_input;
	fprintf(stderr, "%s\n", user_input);
	fprintf(stderr, "%*s", pos, "");
	fprintf(stderr, "^ %s\n", msg);
	exit(1);
}

// tokenize a string pointed by user_input and save them to tokens
void tokenize() {
	char *p = user_input;

	int i = 0;
	while (*p) {
		// skip whitespace
		if (isspace(*p)) {
			p++;
			continue;
		}

		if (*p == '+' || *p == '-') {
			tokens[i].ty = *p;
			tokens[i].input = p;
			i++;
			p++;
			continue;
		}

		if (isdigit(*p)) {
			tokens[i].ty = TK_NUM;
			tokens[i].input = p;
			tokens[i].val = strtol(p, &p, 10);
			i++;
			continue;
		}

		error_at(p, "cannot tokenize");
	}

	tokens[i].ty = TK_EOF;
	tokens[i].input = p;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <number>\n", argv[0]);
		return 1;
	}

	// tokenize
	user_input = argv[1];
	tokenize();

	// output first part assembry
	printf(".intel_syntax noprefix\n");
	printf(".global main\n");
	printf("main:\n");

	// the first expression must be a number
	if (tokens[0].ty != TK_NUM) {
		error_at(tokens[0].input, "not a number");
	}
	printf("  mov rax, %d\n", tokens[0].val);

	int i = 1;
	while (tokens[i].ty != TK_EOF) {
		if (tokens[i].ty == '+') {
			i++;
			if (tokens[i].ty != TK_NUM) {
				error_at(tokens[i].input, "not a number");
			}
			printf("  add rax, %d\n", tokens[i].val);
			i++;
			continue;
		}

		if (tokens[i].ty == '-') {
			i++;
			if (tokens[i].ty != TK_NUM) {
				error_at(tokens[i].input, "not a number");
			}
			printf("  sub rax, %d\n", tokens[i].val);
			i++;
			continue;
		}

		error_at(tokens[i].input, "unexpected token");
	}
	printf("  ret\n");
	return 0;
}
