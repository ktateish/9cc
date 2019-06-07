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

void dump_token(Token *t) {
	fprintf(stderr, "--\n");
	fprintf(stderr, "Token: ");
	switch (t->ty) {
		case TK_IDENT:
			fprintf(stderr, "IDENTIFIER\n");
			fprintf(stderr, "Name: %c\n", t->input[0]);
			break;
		case TK_EQ:
			fprintf(stderr, "'=='\n");
			break;
		case TK_NE:
			fprintf(stderr, "'!='\n");
			break;
		case TK_LE:
			fprintf(stderr, "'<='\n");
			break;
		case TK_GE:
			fprintf(stderr, "'>='\n");
			break;
		case TK_EOF:
			fprintf(stderr, "EOF\n");
			break;
		case TK_NUM:
			fprintf(stderr, "NUMBER\n");
			fprintf(stderr, "Value: %d\n", t->val);
			break;
		default:
			fprintf(stderr, "%c\n", t->ty);
	}
}

void dump_tokens() {
	for (int i = 0; tokens(i) != NULL; i++) {
		dump_token(tokens(i));
	}
}

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

		if ('a' <= *p && *p <= 'z') {
			push_token(new_token(TK_IDENT, p));
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
		    *p == '(' || *p == ')' || *p == '<' || *p == '>' ||
		    *p == ';' || *p == '=') {
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

Node *new_node_ident(char name) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_IDENT;
	nd->name = name;
	return nd;
}

void dump_node(Node *node, int level) {
	fprintf(stderr, "%*s--\n", level * 2, "");
	fprintf(stderr, "%*sNode: ", level * 2, "");
	switch (node->ty) {
		case ND_IDENT:
			fprintf(stderr, "IDENT\n");
			fprintf(stderr, "%*sName: %c\n", level * 2, "",
				node->name);
			break;
		case ND_EQ:
			fprintf(stderr, "'=='\n");
			break;
		case ND_NE:
			fprintf(stderr, "'!='\n");
			break;
		case ND_LE:
			fprintf(stderr, "'<='\n");
			break;
		case ND_NUM:
			fprintf(stderr, "NUMBER\n");
			fprintf(stderr, "%*sValue: %d\n", level * 2, "",
				node->val);
			break;
		default:
			fprintf(stderr, "%c\n", node->ty);
			break;
	}
}

void dump_node_rec(Node *node, int level) {
	if (node == NULL) {
		return;
	}
	dump_node(node, level);
	dump_node_rec(node->lhs, level + 1);
	dump_node_rec(node->rhs, level + 1);
}

void dump_nodes() {
	for (int i = 0; code(i) != NULL; i++) {
		dump_node_rec(code(i), 0);
	}
}

Vector *code_vec;

void init_code() { code_vec = new_vector(); }

void push_code(Node *t) { vec_push(code_vec, t); }

Node *code(int i) { return code_vec->data[i]; }

int consume(int ty) {
	if (tokens(pos)->ty != ty) {
		return 0;
	}
	pos++;
	return 1;
}

// Syntax:
//   program    = stmt*
//   stmt       = expr ";"
//   expr       = assign
//   assign       = equality (= equality)?
//   equality   = relational ("==" relational | "!=" relational)*
//   relational = add ("<" add | "<=" add | ">" add | ">=" add)*
//   add        = mul ("+" mul | "-" mul)*
//   mul        = unary ("*" unary | "/" unary)*
//   unary      = ("+" | "-")? term
//   term       = num | "(" expr ")"

Node *expr();

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

	if (tokens(pos)->ty == TK_IDENT) {
		return new_node_ident(tokens(pos++)->input[0]);
	}

	error_at(tokens(pos)->input, "invalid token");
	return NULL;
}

Node *unary() {
	if (consume('+')) {
		return term();
	} else if (consume('-')) {
		return new_node('-', new_node_num(0), term());
	}
	return term();
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

Node *assign() {
	Node *node = equality();

	for (;;) {
		if (consume('=')) {
			node = new_node('=', node, equality());
		} else {
			return node;
		}
	}
}

Node *expr() { return assign(); }

Node *stmt() {
	Node *node = expr();
	if (!consume(';')) {
		error_at(tokens(pos)->input, "not ';'");
	}
	return node;
}

void program() {
	init_code();
	while (tokens(pos)->ty != TK_EOF) {
		push_code(stmt());
	}
	push_code(NULL);
}
