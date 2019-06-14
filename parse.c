// vim:set filetype=c tabstop=8 shiftwidth=8 noexpandtab:
#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

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

Node *new_node_define_func(char *name, int nr_params, Var *vars, Node *body) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_DEFINE_FUNC;
	nd->name = name;
	nd->vars = vars;
	nd->nr_params = nr_params;
	nd->body = body;
	return nd;
}

Node *new_node_define_int_var(char *name) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_DEFINE_INT_VAR;
	nd->name = name;
	return nd;
}

Node *new_node_block(Vector *stmts) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_BLOCK;
	nd->stmts = stmts;
	return nd;
}

Node *new_node_funcall(char *name, Vector *args) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_FUNCALL;
	nd->name = name;
	nd->args = args;
	return nd;
}

Node *new_node_if(Node *cond, Node *thenc, Node *elsec) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_IF;
	nd->cond = cond;
	nd->thenc = thenc;
	nd->elsec = elsec;
	return nd;
}

Node *new_node_while(Node *cond, Node *body) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_WHILE;
	nd->cond = cond;
	nd->body = body;
	return nd;
}

Node *new_node_for(Node *init, Node *cond, Node *update, Node *body) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_FOR;
	nd->init = init;
	nd->cond = cond;
	nd->update = update;
	nd->body = body;
	return nd;
}

Node *new_node_num(int val) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_NUM;
	nd->val = val;
	return nd;
}

Node *new_node_ident(char *name) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_IDENT;
	nd->name = name;
	return nd;
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

Var *variables;
Var *variables_sentinel;

Var *new_var(Var *next, char *name, int offset) {
	Var *var = malloc(sizeof(Var));
	var->next = next;
	var->name = name;
	var->offset = offset;
	return var;
}

void init_variables() { variables = variables_sentinel = new_var(NULL, "", 0); }

void var_use(Var *vars) {
	variables = vars;
	Var *p = vars;
	while (p->next != NULL) {
		p = p->next;
	}
	variables_sentinel = p;
}

void var_put(char *name) {
	variables = new_var(variables, name, variables->offset + 8);
}

int var_offset(char *name) {
	if (name == NULL) {
		return variables->offset;
	}
	for (Var *p = variables; p != variables_sentinel; p = p->next) {
		if (strcmp(p->name, name) == 0) {
			return p->offset;
		}
	}
	return -1;
}

// Syntax:
//   program    = definition*
//   definition = define_func
//   define_func = "int" identifier "(" params? ")" "{" stmt* "}"
//   params = "int" expr ("," "int" expr)*
//   stmt       = expr ";"
//		| "{" stmt* "}"
//		| "if" "(" expr ")" stmt ("else" stmt)?
//		| "while" "(" expr ")" stmt
//		| "for" "(" expr? ";" expr? ";" expr? ";" ")" stmt
//		| "return" expr ";"
//		| "int" identifier ";"
//   expr       = assign
//   assign       = equality (= equality)?
//   equality   = relational ("==" relational | "!=" relational)*
//   relational = add ("<" add | "<=" add | ">" add | ">=" add)*
//   add        = mul ("+" mul | "-" mul)*
//   mul        = unary ("*" unary | "/" unary)*
//   unary      = ("+" | "-")? term
//   term       = identifier ("(" (expr ("," expr)*)? ")")? | num | "(" expr ")"
//   identifier = [A-Za-z_][0-9A-Za-z]*

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
		Token *tk = tokens(pos++);
		char *name = tk->name;
		if (!consume('(')) {
			// variables
			if (var_offset(name) == -1) {
				error_at(tk->input, "undefined identifier");
			}
			return new_node_ident(name);
		}

		// function call
		Vector *args = new_vector();
		if (consume(')')) {
			// no arguments
			return new_node_funcall(name, args);
		}

		// have oen or more arguments
		vec_push(args, expr());
		while (!consume(')')) {
			if (!consume(',')) {
				error_at(tokens(pos)->input, "',' not found");
			}
			vec_push(args, expr());
		}
		return new_node_funcall(name, args);
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
			node = new_node('=', node, assign());
		} else {
			return node;
		}
	}
}

Node *expr() { return assign(); }

Node *stmt();

Node *stmt_block() {
	Vector *ss = new_vector();
	while (!consume('}')) {
		vec_push(ss, stmt());
	}
	return new_node_block(ss);
}

Node *stmt_if() {
	if (!consume('(')) {
		error_at(tokens(pos)->input, "not '('");
	}
	Node *cond = expr();
	if (!consume(')')) {
		error_at(tokens(pos)->input, "not ')'");
	}
	Node *thenc = stmt();
	Node *elsec = NULL;
	if (consume(TK_ELSE)) {
		elsec = stmt();
	}
	return new_node_if(cond, thenc, elsec);
}

Node *stmt_while() {
	if (!consume('(')) {
		error_at(tokens(pos)->input, "not '('");
	}
	Node *cond = expr();
	if (!consume(')')) {
		error_at(tokens(pos)->input, "not ')'");
	}
	return new_node_while(cond, stmt());
}

Node *stmt_for() {
	if (!consume('(')) {
		error_at(tokens(pos)->input, "not '('");
	}

	Node *init = NULL;
	if (!consume(';')) {
		init = expr();
		if (!consume(';')) {
			error_at(tokens(pos)->input, "not ';'");
		}
	}

	Node *cond = NULL;
	if (!consume(';')) {
		cond = expr();
		if (!consume(';')) {
			error_at(tokens(pos)->input, "not ';'");
		}
	}

	Node *update = NULL;
	if (!consume(')')) {
		update = expr();
		if (!consume(')')) {
			error_at(tokens(pos)->input, "not ')'");
		}
	}

	return new_node_for(init, cond, update, stmt());
}

Node *stmt_return() {
	Node *node = new_node(ND_RETURN, expr(), NULL);
	if (!consume(';')) {
		error_at(tokens(pos)->input, "not ';'");
	}
	return node;
}

Node *stmt_define_int_var() {
	if (tokens(pos)->ty != TK_IDENT) {
		error_at(tokens(pos)->input, "not an identifier");
	}
	char *name = tokens(pos++)->name;
	// variables
	if (var_offset(name) != -1) {
		error_at(tokens(pos)->input, "duplicate definition");
	}
	var_put(name);
	if (!consume(';')) {
		error_at(tokens(pos)->input, "not ';'");
	}
	return new_node_define_int_var(name);
}

Node *stmt() {
	if (consume(TK_INT)) return stmt_define_int_var();
	if (consume('{')) return stmt_block();
	if (consume(TK_IF)) return stmt_if();
	if (consume(TK_WHILE)) return stmt_while();
	if (consume(TK_FOR)) return stmt_for();
	if (consume(TK_RETURN)) return stmt_return();

	Node *node = expr();
	if (!consume(';')) {
		error_at(tokens(pos)->input, "not ';'");
	}
	return node;
}

void params() {
	if (!consume(TK_INT)) {
		error_at(tokens(pos)->input, "not 'int'");
	}
	if (tokens(pos)->ty != TK_IDENT) {
		error_at(tokens(pos)->input, "not an identifier");
	}
	var_put(tokens(pos++)->name);

	while (!consume(')')) {
		if (!consume(',')) {
			error_at(tokens(pos)->input, "',' not found");
		}
		if (!consume(TK_INT)) {
			error_at(tokens(pos)->input, "not 'int'");
		}
		if (tokens(pos)->ty != TK_IDENT) {
			error_at(tokens(pos)->input, "not an identifier");
		}
		var_put(tokens(pos++)->name);
	}
}

Node *define_func() {
	if (!consume(TK_INT)) {
		error_at(tokens(pos)->input, "not 'int'");
	}

	if (tokens(pos)->ty != TK_IDENT) {
		error_at(tokens(pos)->input, "not an identifier");
	}

	char *name = tokens(pos++)->name;
	init_variables();

	if (!consume('(')) {
		error_at(tokens(pos)->input, "not '('");
	}
	if (!consume(')')) {
		params();
	}
	int nr_params = var_offset(NULL) / 8;

	if (!consume('{')) {
		error_at(tokens(pos)->input, "not '{'");
	}
	Vector *body = new_vector();
	while (!consume('}')) {
		vec_push(body, stmt());
	}

	Node *node = new_node_define_func(name, nr_params, variables,
					  new_node_block(body));
	variables = NULL;
	return node;
}

Node *definition() { return define_func(); }

void program() {
	init_code();
	while (tokens(pos)->ty != TK_EOF) {
		push_code(definition());
	}
	push_code(NULL);
}
