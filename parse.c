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

// Type types
Type *new_type_int() {
	Type *tp = malloc(sizeof(Type));
	tp->ty = TP_INT;
	return tp;
}

Type *new_type_ptr(Type *ptr_to) {
	Type *tp = malloc(sizeof(Type));
	tp->ty = TP_POINTER;
	tp->ptr_to = ptr_to;
	return tp;
}

Type *new_type_function(Vector *params, Type *returning) {
	Type *tp = malloc(sizeof(Type));
	tp->ty = TP_FUNCTION;
	tp->params = params;
	tp->returning = returning;
	return tp;
}

Type *new_type_undetermined() {
	Type *tp = malloc(sizeof(TP_UNDETERMINED));
	tp->ty = TP_UNDETERMINED;
	return tp;
}

char *type_name(Type *tp) {
	if (tp == NULL) return "unknown";
	if (tp->ty == TP_INT) return "int";
	if (tp->ty == TP_POINTER) {
		char *src = type_name(tp->ptr_to);
		char *s = malloc(strlen(src) + 2);
		sprintf(s, "*%s", src);
		return s;
	}
	if (tp->ty == TP_FUNCTION) {
		char *returning = type_name(tp->returning);

		Vector *params = new_vector();
		for (int i = 0; i < tp->params->len; i++) {
			Type *p = tp->params->data[i];
			char *s = type_name(p);
			vec_push(params, s);
		}
		char *param_str = "";
		if (0 < tp->params->len) {
			param_str = string_join(params, ", ");
		}

		char *s = malloc(strlen("func() ") + strlen(param_str) +
				 strlen(returning) + 1);
		sprintf(s, "func(%s) %s", param_str, returning);
		return s;
	}
	if (tp->ty == TP_UNDETERMINED) return "undetermined";
	return "unknown";
}

// node types
Node *new_node(int node_type, Node *lhs, Node *rhs) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = node_type;
	nd->lhs = lhs;
	nd->rhs = rhs;
	nd->tp = new_type_undetermined();
	return nd;
}

Node *new_node_declare_func(char *name, Type *tp) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_DECLARE_FUNC;
	nd->name = name;
	nd->tp = tp;
	return nd;
}

Node *new_node_define_func(char *name, Type *tp, Vector *params, Scope *scope,
			   Node *body) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_DEFINE_FUNC;
	nd->name = name;
	nd->scope = scope;
	nd->params = params;
	nd->body = body;
	nd->tp = tp;
	return nd;
}

Node *new_node_define_int_var(char *name, Type *tp, char *input) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_DEFINE_INT_VAR;
	nd->name = name;
	nd->tp = tp;
	nd->input = input;
	return nd;
}

Node *new_node_block(Vector *stmts) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_BLOCK;
	nd->stmts = stmts;
	nd->tp = new_type_undetermined();
	return nd;
}

Node *new_node_funcall(char *name, Vector *args) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_FUNCALL;
	nd->name = name;
	nd->args = args;
	nd->tp = new_type_undetermined();
	return nd;
}

Node *new_node_if(Node *cond, Node *thenc, Node *elsec) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_IF;
	nd->cond = cond;
	nd->thenc = thenc;
	nd->elsec = elsec;
	nd->tp = new_type_undetermined();
	return nd;
}

Node *new_node_while(Node *cond, Node *body) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_WHILE;
	nd->cond = cond;
	nd->body = body;
	nd->tp = new_type_undetermined();
	return nd;
}

Node *new_node_for(Node *init, Node *cond, Node *update, Node *body) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_FOR;
	nd->init = init;
	nd->cond = cond;
	nd->update = update;
	nd->body = body;
	nd->tp = new_type_undetermined();
	return nd;
}

Node *new_node_num(int val) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_NUM;
	nd->val = val;
	nd->tp = new_type_undetermined();
	return nd;
}

Node *new_node_ident(char *name, char *input) {
	Node *nd = malloc(sizeof(Node));
	nd->ty = ND_IDENT;
	nd->name = name;
	nd->input = input;
	nd->tp = new_type_undetermined();
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
//		| "int" "*"* identifier ";"
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
	if (tokens(pos)->ty != TK_IDENT) {
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
		return identifier();
	}

	if (consume('*')) {
		return new_node(ND_DEREF, term(), NULL);
	}

	if (consume('&')) {
		return new_node(ND_ENREF, identifier(), NULL);
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

	if (tokens(pos)->ty != TK_IDENT) {
		error_at(tokens(pos)->input, "not an identifier");
	}
	Token *tk = tokens(pos++);
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

	if (tokens(pos)->ty != TK_IDENT) {
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

		if (tokens(pos)->ty != TK_IDENT) {
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

	if (tokens(pos)->ty != TK_IDENT) {
		error_at(tokens(pos)->input, "not an identifier");
	}

	char *name = tokens(pos++)->name;

	Vector *params = define_func_params();

	Vector *param_types = new_vector();
	for (int i = 0; i < params->len; i++) {
		Node *nd = params->data[i];
		vec_push(param_types, nd->tp);
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
	while (tokens(pos)->ty != TK_EOF) {
		push_code(definition());
	}
	push_code(NULL);
}
