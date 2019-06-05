#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Vector
typedef struct {
	void **data;
	int capacity;
	int len;
} Vector;

Vector *new_vector() {
	Vector *vec = malloc(sizeof(Vector));
	vec->data = malloc(sizeof(void *) * 16);
	vec->capacity = 16;
	vec->len = 0;
	return vec;
}

void vec_push(Vector *vec, void *elem) {
	if (vec->capacity == vec->len) {
		vec->capacity *= 2;
		vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
	}
	vec->data[vec->len++] = elem;
}

// token types
enum TokenType {
	TK_NUM = 256,  // integer
	TK_EOF,
	TK_EQ,
	TK_NE,
	TK_LE,
	TK_GE,
};

// token
typedef struct {
	int ty;
	int val;
	char *input;
} Token;

Token *new_token(int ty, char *input) {
	Token *t = malloc(sizeof(Token));
	t->ty = ty;
	t->input = input;
	return t;
}

Token *new_token_num(int val, char *input) {
	Token *t = malloc(sizeof(Token));
	t->ty = TK_NUM;
	t->val = val;
	t->input = input;
	return t;
}

// input
char *user_input;

// tokenized results
Vector *token_vec;

void init_tokens() { token_vec = new_vector(); }

void push_token(Token *t) { vec_push(token_vec, t); }

Token *tokens(int i) { return token_vec->data[i]; }

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
void tokenize(char *p) {
	user_input = p;

	while (*p) {
		// skip whitespace
		if (isspace(*p)) {
			p++;
			continue;
		}

		if (*p == '=' && *(p + 1) == '=') {
			push_token(new_token(TK_EQ, p));
			p += 2;
			continue;
		}

		if (*p == '!' && *(p + 1) == '=') {
			push_token(new_token(TK_NE, p));
			p += 2;
			continue;
		}

		if (*p == '<' && *(p + 1) == '=') {
			push_token(new_token(TK_LE, p));
			p += 2;
			continue;
		}

		if (*p == '>' && *(p + 1) == '=') {
			push_token(new_token(TK_GE, p));
			p += 2;
			continue;
		}

		if (*p == '+' || *p == '-' || *p == '*' || *p == '/' ||
		    *p == '(' || *p == ')' || *p == '<' || *p == '>') {
			push_token(new_token(*p, p));
			p++;
			continue;
		}

		if (isdigit(*p)) {
			push_token(new_token_num(strtol(p, &p, 10), p));
			continue;
		}

		error_at(p, "cannot tokenize");
	}

	push_token(new_token(TK_EOF, p));
}

//
// Parser
//

// position for parser
int pos;

// node types
enum NodeType {
	ND_NUM = 256,
	ND_EQ,
	ND_NE,
	ND_LE,
};

typedef struct Node {
	int ty;  // operator or ND_NUM
	struct Node *lhs, *rhs;
	int val;  // for ND_NUM
} Node;

Node *new_node(int node_type, Node *lhs, Node *rhs) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = node_type;
	nd->lhs = lhs;
	nd->rhs = rhs;
	return nd;
}

Node *new_node_num(int val) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_NUM;
	nd->val = val;
	return nd;
}

int consume(int ty) {
	if (tokens(pos)->ty != ty) {
		return 0;
	}
	pos++;
	return 1;
}

// Syntax:
//   expr       = equality
//   equality   = relational ("==" relational | "!=" relational)*
//   relational = add ("<" add | "<=" add | ">" add | ">=" add)*
//   add        = mul ("+" mul | "-" mul)*
//   mul        = unary ("*" unary | "/" unary)*
//   unary      = ("+" | "-")? term
//   term       = num | "(" expr ")"

Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *term();

Node *expr() { return equality(); }

Node *equality() {
	Node *node = relational();

	for (;;) {
		if (consume(TK_EQ)) {
			node = new_node(ND_EQ, node, relational());
		} else if (consume(TK_NE)) {
			node = new_node(ND_NE, node, relational());
		} else {
			return node;
		}
	}
}

Node *relational() {
	Node *node = add();

	for (;;) {
		if (consume('<')) {
			node = new_node('<', node, add());
		} else if (consume(TK_LE)) {
			node = new_node(ND_LE, node, add());
		} else if (consume('>')) {
			node = new_node('<', add(), node);
		} else if (consume(TK_GE)) {
			node = new_node(ND_LE, add(), node);
		} else {
			return node;
		}
	}
}

Node *add() {
	Node *node = mul();

	for (;;) {
		if (consume('+')) {
			node = new_node('+', node, mul());
		} else if (consume('-')) {
			node = new_node('-', node, mul());
		} else {
			return node;
		}
	}
}

Node *mul() {
	Node *node = unary();

	for (;;) {
		if (consume('*')) {
			node = new_node('*', node, unary());
		} else if (consume('/')) {
			node = new_node('/', node, unary());
		} else {
			return node;
		}
	}
}

Node *unary() {
	if (consume('+')) {
		return term();
	} else if (consume('-')) {
		return new_node('-', new_node_num(0), term());
	}
	return term();
}

Node *term() {
	if (consume('(')) {
		Node *node = expr();
		if (!consume(')')) {
			error_at(tokens(pos)->input, "close ')' not found");
		}
		return node;
	}

	if (tokens(pos)->ty == TK_NUM) {
		return new_node_num(tokens(pos++)->val);
	}

	error_at(tokens(pos)->input, "invalid token");
}

void gen(Node *node) {
	if (node->ty == ND_NUM) {
		printf("  push %d\n", node->val);
		return;
	}

	gen(node->lhs);
	gen(node->rhs);

	printf("  pop rdi\n");
	printf("  pop rax\n");

	switch (node->ty) {
		case '+':
			printf("  add rax, rdi\n");
			break;
		case '-':
			printf("  sub rax, rdi\n");
			break;
		case '*':
			printf("  imul rdi\n");
			break;
		case '/':
			printf("  cqo\n");
			printf("  idiv rdi\n");
			break;
		case ND_EQ:
			printf("  cmp rax, rdi\n");
			printf("  sete al\n");
			printf("  movzb rax, al\n");
			break;
		case ND_NE:
			printf("  cmp rax, rdi\n");
			printf("  setne al\n");
			printf("  movzb rax, al\n");
			break;
		case '<':
			printf("  cmp rax, rdi\n");
			printf("  setl al\n");
			printf("  movzb rax, al\n");
			break;
		case ND_LE:
			printf("  cmp rax, rdi\n");
			printf("  setle al\n");
			printf("  movzb rax, al\n");
			break;
	}
	printf("  push rax\n");
}

// Test
void expect(int line, int expected, int actual) {
	if (expected == actual) return;
	fprintf(stderr, "%d: %d expected, but got %d\n", line, expected,
		actual);
	exit(1);
}

void runtest() {
	Vector *vec = new_vector();
	expect(__LINE__, 0, vec->len);

	for (int i = 0; i < 100; i++) vec_push(vec, (void *)i);

	expect(__LINE__, 100, vec->len);
	expect(__LINE__, 0, (long)vec->data[0]);
	expect(__LINE__, 50, (long)vec->data[50]);
	expect(__LINE__, 99, (long)vec->data[99]);

	printf("OK\n");
}

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

	Node *node = expr();

	// output first part assembry
	printf(".intel_syntax noprefix\n");
	printf(".global main\n");
	printf("main:\n");

	// Generate code
	gen(node);

	// The value of the expression will be on the top of the stack
	// load it to rax and let it be return value.
	printf("  pop rax\n");
	printf("  ret\n");

	return 0;
}
