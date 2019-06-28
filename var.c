// vim:set filetype=c tabstop=8 shiftwidth=8 noexpandtab:
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

Scope *global_scope;
Scope *scope;
int max_offset;

Var *new_var(Var *next, char *name, int offset, Type *type) {
	Var *var = malloc(sizeof(Var));
	var->next = next;
	var->name = name;
	var->offset = offset;
	var->type = type;
	return var;
}

void var_use(Node *node) { scope = node->scope; }

void var_put(char *name, Type *tp) {
	int add = 8;
	if (scope == global_scope) {
		add = 0;
	}
	if (tp->kind == TP_ARRAY) {
		add = 8 * tp->array_size;
	}
	scope->vars = new_var(scope->vars, name, scope->vars->offset + add, tp);
	if (max_offset < scope->vars->offset) {
		max_offset = scope->vars->offset;
	}
}

Var *var_get(char *name) {
	if (name == NULL) {
		return scope->vars;
	}
	for (Scope *s = scope; s != NULL; s = s->next) {
		for (Var *p = s->vars; p != s->sentinel; p = p->next) {
			if (strcmp(p->name, name) == 0) {
				return p;
			}
		}
	}
	return NULL;
}

int var_duplicated(char *name) {
	for (Var *p = scope->vars; p != scope->sentinel; p = p->next) {
		if (strcmp(p->name, name) == 0) {
			return 1;
		}
	}
	return 0;
}

Scope *new_scope(Scope *next) {
	Scope *s = malloc(sizeof(Scope));
	s->next = next;
	int offset = 0;
	if (next != NULL) {
		offset = next->vars->offset;
	}
	s->vars = s->sentinel = new_var(NULL, "", offset, NULL);
	return s;
}

Scope *scope_use(Scope *sc) {
	Scope *old = scope;
	scope = sc;
	return old;
}

void init_global_scope() { scope = global_scope = new_scope(NULL); }

void init_function_scope() {
	scope = new_scope(global_scope);
	max_offset = 0;
}

void set_function_scope(Node *node) {
	if (node->kind != ND_DEFINE_FUNC) {
		error("Cannot set function scope to non function node");
	}
	node->scope = scope;
	node->max_offset = max_offset;
	scope = global_scope;
}

void set_scope(Node *node) {
	if (node->kind != ND_BLOCK) {
		error("invalid node to set scope");
	}
	node->scope = scope;
}

void push_scope() {
	Scope *ns = new_scope(scope);
	scope = ns;
}

void pop_scope() {
	Scope *prev = scope->next;
	scope = prev;
}
