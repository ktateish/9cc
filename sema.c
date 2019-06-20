// vim:set filetype=c tabstop=8 shiftwidth=8 noexpandtab:
#include <stdio.h>

#include "9cc.h"

extern Var *variables;

void sema_rec(Node *node) {
	if (node == NULL) return;
	switch (node->ty) {
		case ND_DEFINE_INT_VAR:
			if (var_offset(node->name) != -1) {
				error_at(node->input, "duplicate definition");
			}
			var_put(node->name);
			return;
		case ND_IDENT:
			if (var_offset(node->name) == -1) {
				error_at(node->input, "unknown variable");
			}
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
