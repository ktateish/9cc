// vim:set filetype=c tabstop=8 shiftwidth=8 noexpandtab:
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
	nd->kind = node_type;
	nd->lhs = lhs;
	nd->rhs = rhs;
	nd->type = new_type_undetermined();
	return nd;
}

Node *new_node_declare_func(char *name, Type *type) {
	Node *nd = malloc(sizeof(Node));
	nd->kind = ND_DECLARE_FUNC;
	nd->name = name;
	nd->type = type;
	return nd;
}

Node *new_node_define_func(char *name, Type *type, Vector *params, Scope *scope,
			   Node *body) {
	Node *nd = malloc(sizeof(Node));
	nd->kind = ND_DEFINE_FUNC;
	nd->name = name;
	nd->scope = scope;
	nd->params = params;
	nd->body = body;
	nd->type = type;
	return nd;
}

Node *new_node_define_int_var(char *name, Type *type, char *input) {
	Node *nd = malloc(sizeof(Node));
	nd->kind = ND_DEFINE_INT_VAR;
	nd->name = name;
	nd->type = type;
	nd->input = input;
	return nd;
}

Node *new_node_block(Vector *stmts) {
	Node *nd = malloc(sizeof(Node));
	nd->kind = ND_BLOCK;
	nd->stmts = stmts;
	nd->type = new_type_undetermined();
	return nd;
}

Node *new_node_funcall(char *name, Vector *args) {
	Node *nd = malloc(sizeof(Node));
	nd->kind = ND_FUNCALL;
	nd->name = name;
	nd->args = args;
	nd->type = new_type_undetermined();
	return nd;
}

Node *new_node_if(Node *cond, Node *thenc, Node *elsec) {
	Node *nd = malloc(sizeof(Node));
	nd->kind = ND_IF;
	nd->cond = cond;
	nd->thenc = thenc;
	nd->elsec = elsec;
	nd->type = new_type_undetermined();
	return nd;
}

Node *new_node_while(Node *cond, Node *body) {
	Node *nd = malloc(sizeof(Node));
	nd->kind = ND_WHILE;
	nd->cond = cond;
	nd->body = body;
	nd->type = new_type_undetermined();
	return nd;
}

Node *new_node_for(Node *init, Node *cond, Node *update, Node *body) {
	Node *nd = malloc(sizeof(Node));
	nd->kind = ND_FOR;
	nd->init = init;
	nd->cond = cond;
	nd->update = update;
	nd->body = body;
	nd->type = new_type_undetermined();
	return nd;
}

Node *new_node_num(int val) {
	Node *nd = malloc(sizeof(Node));
	nd->kind = ND_NUM;
	nd->val = val;
	nd->type = new_type_undetermined();
	return nd;
}

Node *new_node_ident(char *name, char *input) {
	Node *nd = malloc(sizeof(Node));
	nd->kind = ND_IDENT;
	nd->name = name;
	nd->input = input;
	nd->type = new_type_undetermined();
	return nd;
}

Node *node_dup(Node *node) {
	Node *nn = malloc(sizeof(Node));
	*nn = *node;
	return nn;
}

Vector *code_vec;

void init_code() { code_vec = new_vector(); }

void push_code(Node *t) { vec_push(code_vec, t); }

Node *code(int i) { return vec_at(code_vec, i); }

int consume(int kind) {
	if (tokens(pos)->kind != kind) {
		return 0;
	}
	pos++;
	return 1;
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
//		| "int" "*"* identifier ("[" num "]")?";"
//   expr       = assign
//   assign       = equality (= equality)?
//   equality   = relational ("==" relational | "!=" relational)*
//   relational = add ("<" add | "<=" add | ">" add | ">=" add)*
//   add        = mul ("+" mul | "-" mul)*
//   mul        = unary ("*" unary | "/" unary)*
//   unary      = ("+" | "-")? term
//   term       = refspec? identifier ("(" (expr ("," expr)*)? ")")?
//   		| num
//   		| "(" expr ")"
//   identifier = [A-Za-z_][0-9A-Za-z]*
//   refspec    = "*"* | "&"

Node *expr();

Node *identifier() {
	if (tokens(pos)->kind != TK_IDENT) {
		error_at(tokens(pos)->input, "invalid token");
	}
	Token *tk = tokens(pos++);
	char *name = tk->name;
	char *input = tk->input;
	if (!consume('(')) {
		// variables
		return new_node_ident(name, input);
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

Node *array_index(Node *pre) {
	if (!consume('[')) {
		return pre;
	}

	Node *node = expr();
	if (!consume(']')) {
		error_at(tokens(pos)->input, "close ']' not found");
	}
	return new_node(ND_DEREF, new_node('+', pre, node), NULL);
}

Node *term() {
	if (consume('(')) {
		Node *node = expr();
		if (!consume(')')) {
			error_at(tokens(pos)->input, "close ')' not found");
		}
		return node;
	}

	if (tokens(pos)->kind == TK_NUM) {
		Node *node = new_node_num(tokens(pos++)->val);
		return array_index(node);
	}

	if (tokens(pos)->kind == TK_IDENT) {
		Node *node = identifier();
		return array_index(node);
	}

	error_at(tokens(pos)->input, "invalid token");
	return NULL;
}

Node *unary() {
	if (consume('+')) {
		return term();
	} else if (consume('-')) {
		return new_node('-', new_node_num(0), term());
	} else if (consume('*')) {
		return new_node(ND_DEREF, unary(), NULL);
	} else if (consume('&')) {
		return new_node(ND_ENREF, term(), NULL);
	} else if (consume(TK_SIZEOF)) {
		return new_node(ND_SIZEOF, unary(), NULL);
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
	Type *tp = new_type_int();

	while (consume('*')) {
		tp = new_type_ptr(tp);
	}

	if (tokens(pos)->kind != TK_IDENT) {
		error_at(tokens(pos)->input, "not an identifier");
	}
	Token *tk = tokens(pos++);

	if (consume('[')) {
		int array_size;
		if (tokens(pos)->kind != TK_NUM) {
			error_at(tokens(pos)->input, "not a number");
		}
		array_size = tokens(pos++)->val;
		tp = new_type_array(tp, array_size);
		if (!consume(']')) {
			error_at(tokens(pos)->input, "not ']'");
		}
	}
	char *name = tk->name;
	char *input = tk->input;
	// variables
	if (!consume(';')) {
		error_at(tokens(pos)->input, "not ';'");
	}
	return new_node_define_int_var(name, tp, input);
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

Vector *define_func_params() {
	Vector *prms = new_vector();

	if (!consume('(')) {
		error_at(tokens(pos)->input, "not '('");
	}

	if (consume(')')) {
		return prms;
	}

	if (!consume(TK_INT)) {
		error_at(tokens(pos)->input, "not 'int'");
	}
	Type *tp = new_type_int();

	while (consume('*')) {
		tp = new_type_ptr(tp);
	}

	if (tokens(pos)->kind != TK_IDENT) {
		error_at(tokens(pos)->input, "not an identifier");
	}

	Token *tk = tokens(pos++);
	Node *node = new_node_define_int_var(tk->name, tp, tk->input);
	vec_push(prms, node);

	while (!consume(')')) {
		if (!consume(',')) {
			error_at(tokens(pos)->input, "',' not found");
		}
		if (!consume(TK_INT)) {
			error_at(tokens(pos)->input, "not 'int'");
		}

		tp = new_type_int();

		while (consume('*')) {
			tp = new_type_ptr(tp);
		}

		if (tokens(pos)->kind != TK_IDENT) {
			error_at(tokens(pos)->input, "not an identifier");
		}
		tk = tokens(pos++);
		node = new_node_define_int_var(tk->name, tp, tk->input);
		vec_push(prms, node);
	}
	return prms;
}

Node *define_func() {
	if (!consume(TK_INT)) {
		error_at(tokens(pos)->input, "not 'int'");
	}

	Type *returning = new_type_int();

	while (consume('*')) {
		returning = new_type_ptr(returning);
	}

	if (tokens(pos)->kind != TK_IDENT) {
		error_at(tokens(pos)->input, "not an identifier");
	}

	char *name = tokens(pos++)->name;

	Vector *params = define_func_params();

	Vector *param_types = new_vector();
	for (int i = 0; i < vec_len(params); i++) {
		Node *nd = vec_at(params, i);
		vec_push(param_types, nd->type);
	}

	Type *tp = new_type_function(param_types, returning);

	if (consume(';')) {
		return new_node_declare_func(name, tp);
	}

	if (!consume('{')) {
		error_at(tokens(pos)->input, "not '{'");
	}
	Vector *body = new_vector();
	while (!consume('}')) {
		vec_push(body, stmt());
	}

	Node *node =
	    new_node_define_func(name, tp, params, NULL, new_node_block(body));
	return node;
}

Node *definition() { return define_func(); }

void program() {
	init_code();
	while (tokens(pos)->kind != TK_EOF) {
		push_code(definition());
	}
	push_code(NULL);
}
