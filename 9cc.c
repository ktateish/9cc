#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

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
void tokenize(char *p) {
	user_input = p;

	int i = 0;
	while (*p) {
		// skip whitespace
		if (isspace(*p)) {
			p++;
			continue;
		}

		if (*p == '=' && *(p + 1) == '=') {
			tokens[i].ty = TK_EQ;
			tokens[i].input = p;
			i++;
			p += 2;
			continue;
		}

		if (*p == '!' && *(p + 1) == '=') {
			tokens[i].ty = TK_NE;
			tokens[i].input = p;
			i++;
			p += 2;
			continue;
		}

		if (*p == '<' && *(p + 1) == '=') {
			tokens[i].ty = TK_LE;
			tokens[i].input = p;
			i++;
			p += 2;
			continue;
		}

		if (*p == '>' && *(p + 1) == '=') {
			tokens[i].ty = TK_GE;
			tokens[i].input = p;
			i++;
			p += 2;
			continue;
		}

		if (*p == '+' || *p == '-' || *p == '*' || *p == '/' ||
		    *p == '(' || *p == ')' || *p == '<' || *p == '>') {
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
	if (tokens[pos].ty != ty) {
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
			error_at(tokens[pos].input, "close ')' not found");
		}
		return node;
	}

	if (tokens[pos].ty == TK_NUM) {
		return new_node_num(tokens[pos++].val);
	}

	error_at(tokens[pos].input, "invalid token");
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

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <number>\n", argv[0]);
		return 1;
	}

	// tokenize
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
