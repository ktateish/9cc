// vim:set filetype=c tabstop=8 shiftwidth=8 noexpandtab:
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

Var *variables;
Var *variables_sentinel;

Var *new_var(Var *next, char *name, int offset, Type *tp) {
	Var *var = malloc(sizeof(Var));
	var->next = next;
	var->name = name;
	var->offset = offset;
	var->tp = tp;
	return var;
}

void init_variables() {
	variables = variables_sentinel = new_var(NULL, "", 0, NULL);
}

void var_use(Var *vars) {
	variables = vars;
	Var *p = vars;
	while (p->next != NULL) {
		p = p->next;
	}
	variables_sentinel = p;
}

void var_put(char *name, Type *tp) {
	variables = new_var(variables, name, variables->offset + 8, tp);
}

Var *var_get(char *name) {
	if (name == NULL) {
		return variables;
	}
	for (Var *p = variables; p != variables_sentinel; p = p->next) {
		if (strcmp(p->name, name) == 0) {
			return p;
		}
	}
	return NULL;
}

void sema_rec(Node *node) {
	if (node == NULL) return;

	Var *v;
	switch (node->ty) {
		case ND_DEFINE_INT_VAR:
			if (var_get(node->name) != NULL) {
				error_at(node->input, "duplicate definition");
			}
			var_put(node->name, node->tp);
			return;
		case ND_IDENT:
			v = var_get(node->name);
			if (v == NULL) {
				error_at(node->input, "unknown variable");
			}
			node->tp = v->tp;
			return;
		case ND_BLOCK:
			for (int i = 0; i < node->stmts->len; i++) {
				sema_rec(node->stmts->data[i]);
			}
			return;
		case ND_FUNCALL:
			for (int i = 0; i < node->args->len; i++) {
				sema_rec(node->args->data[i]);
			}
			return;
		case ND_IF:
			sema_rec(node->cond);
			sema_rec(node->thenc);
			sema_rec(node->elsec);
			return;
		case ND_WHILE:
			sema_rec(node->cond);
			sema_rec(node->body);
			return;
		case ND_FOR:
			sema_rec(node->init);
			sema_rec(node->cond);
			sema_rec(node->update);
			sema_rec(node->body);
			return;
		default:
			sema_rec(node->lhs);
			sema_rec(node->rhs);
			return;
	}
}

void sema_toplevel(Node *node) {
	if (node->ty == ND_DEFINE_FUNC) {
		init_variables();
		for (int i = 0; i < node->params->len; i++) {
			sema_rec(node->params->data[i]);
		}
		sema_rec(node->body);
		node->vars = variables;
	}
}

void sema() {
	for (int i = 0; code(i) != NULL; i++) {
		Node *node = code(i);
		sema_toplevel(node);
	}
}
