// vim:set filetype=c tabstop=8 shiftwidth=8 noexpandtab:
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "9cc.h"

// token
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
	return NULL;
}
