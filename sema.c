// vim:set filetype=c tabstop=8 shiftwidth=8 noexpandtab:
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

Scope *global_scope;
Scope *scope;
int max_offset;

Var *new_var(Var *next, char *name, int offset, Type *tp) {
	Var *var = malloc(sizeof(Var));
	var->next = next;
	var->name = name;
	var->offset = offset;
	var->tp = tp;
	return var;
}

void var_use(Node *node) { scope = node->scope; }

void var_put(char *name, Type *tp) {
	int add = 8;
	if (scope == global_scope) {
		add = 0;
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
	if (node->ty != ND_DEFINE_FUNC) {
		error("Cannot set function scope to non function node");
	}
	node->scope = scope;
	node->max_offset = max_offset;
	scope = global_scope;
}

void set_scope(Node *node) {
	if (node->ty != ND_BLOCK) {
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

Type *result_type_add(Type *lhs, Type *rhs) {
	if (lhs->ty == TP_INT && rhs->ty == TP_INT) return lhs;
	if (lhs->ty == TP_POINTER && rhs->ty == TP_INT) return lhs;
	if (lhs->ty == TP_INT && rhs->ty == TP_POINTER) return rhs;
	if (lhs->ty == TP_POINTER && rhs->ty == TP_POINTER) return NULL;
	return NULL;
}

Type *result_type_sub(Type *lhs, Type *rhs) {
	if (lhs->ty == TP_INT && rhs->ty == TP_INT) return lhs;
	if (lhs->ty == TP_POINTER && rhs->ty == TP_INT) return lhs;
	if (lhs->ty == TP_INT && rhs->ty == TP_POINTER) return rhs;
	if (lhs->ty == TP_POINTER && rhs->ty == TP_POINTER) return rhs;
	return NULL;
}

Type *result_type_mul(Type *lhs, Type *rhs) {
	if (lhs->ty == TP_INT && rhs->ty == TP_INT) return lhs;
	return NULL;
}

Type *result_type_div(Type *lhs, Type *rhs) {
	if (lhs->ty == TP_INT && rhs->ty == TP_INT) return lhs;
	return NULL;
}

int assignable(Type *lhs, Type *rhs) {
	if (lhs->ty == TP_POINTER && rhs->ty == TP_POINTER) {
		return assignable(lhs->ptr_to, rhs->ptr_to);
	}
	return lhs->ty == rhs->ty;
}

Type *result_type_assign(Type *lhs, Type *rhs) {
	if (assignable(lhs, rhs)) return lhs;
	return NULL;
}

Type *result_type_eq(Type *lhs, Type *rhs) {
	if (lhs->ty == TP_INT && rhs->ty == TP_INT) return lhs;
	return NULL;
}

Type *result_type_enref(Type *tp) { return new_type_ptr(tp); }

Type *result_type_deref(Type *tp) {
	if (tp->ty != TP_POINTER) {
		return NULL;
	}
	return tp->ptr_to;
}

Type *result_type(int op, Type *lhs, Type *rhs) {
	if (op == '+') return result_type_add(lhs, rhs);
	if (op == '-') return result_type_sub(lhs, rhs);
	if (op == '*') return result_type_mul(lhs, rhs);
	if (op == '/') return result_type_div(lhs, rhs);
	if (op == '=') return result_type_assign(lhs, rhs);
	if (op == ND_EQ || op == ND_NE || op == ND_LE || op == '<')
		return result_type_eq(lhs, rhs);
	if (op == ND_ENREF) return result_type_enref(lhs);
	if (op == ND_DEREF) return result_type_deref(lhs);
	return NULL;
}

void sema_rec(Node *node) {
	if (node == NULL) return;

	Var *v;
	Type *tp;
	if (node->ty == ND_DEFINE_INT_VAR) {
		if (var_duplicated(node->name)) {
			error_at(node->input, "duplicate definition");
		}
		var_put(node->name, node->tp);
		return;
	}
	if (node->ty == ND_IDENT) {
		v = var_get(node->name);
		if (v == NULL) {
			error_at(node->input, "unknown variable");
		}
		node->tp = v->tp;
		return;
	}
	if (node->ty == ND_BLOCK) {
		push_scope();
		for (int i = 0; i < node->stmts->len; i++) {
			sema_rec(node->stmts->data[i]);
		}
		set_scope(node);
		pop_scope();
		return;
	}
	if (node->ty == ND_FUNCALL) {
		for (int i = 0; i < node->args->len; i++) {
			sema_rec(node->args->data[i]);
		}
		node->tp = new_type_int();
		return;
	}
	if (node->ty == ND_IF) {
		sema_rec(node->cond);
		sema_rec(node->thenc);
		sema_rec(node->elsec);
		return;
	}
	if (node->ty == ND_WHILE) {
		sema_rec(node->cond);
		sema_rec(node->body);
		return;
	}
	if (node->ty == ND_FOR) {
		sema_rec(node->init);
		sema_rec(node->cond);
		sema_rec(node->update);
		sema_rec(node->body);
		return;
	}
	if (node->ty == ND_ENREF) {
		sema_rec(node->lhs);
		if (node->lhs->ty != ND_IDENT) {
			error("cannot get address of non-identifier");
		}
		tp = result_type_enref(node->lhs->tp);
		node->tp = tp;
		return;
	}
	if (node->ty == ND_DEREF) {
		sema_rec(node->lhs);
		tp = result_type_deref(node->lhs->tp);
		if (tp == NULL) {
			error("cannot derefer non pointer type");
		}
		node->tp = tp;
		return;
	}
	if (node->ty == ND_RETURN) {
		sema_rec(node->lhs);
		tp = node->lhs->tp;
		// XXX: check whether match the returning type of the
		// function
		if (tp == NULL || tp->ty == TP_UNDETERMINED) {
			error("cannot return undetermined type");
		}
		node->tp = tp;
		return;
	}
	if (node->ty == '=') {
		sema_rec(node->lhs);
		sema_rec(node->rhs);
		tp = result_type(node->ty, node->lhs->tp, node->rhs->tp);
		if (tp == NULL) {
			error("type unmatch: lhs: %s, rhs: %s",
			      type_name(node->lhs->tp),
			      type_name(node->rhs->tp));
		}
		node->tp = tp;
		return;
	}
	if (node->ty == ND_NUM) {
		node->tp = new_type_int();
		return;
	}
	sema_rec(node->lhs);
	sema_rec(node->rhs);
	tp = result_type(node->ty, node->lhs->tp, node->rhs->tp);
	if (tp == NULL) {
		error("type unmatch: lhs: %s, rhs: %s",
		      type_name(node->lhs->tp), type_name(node->rhs->tp));
	}
	node->tp = tp;
}

void sema_toplevel(Node *node) {
	if (node->ty == ND_DEFINE_FUNC) {
		init_function_scope();
		for (int i = 0; i < node->params->len; i++) {
			sema_rec(node->params->data[i]);
		}
		sema_rec(node->body);
		set_function_scope(node);
	}
}

void sema() {
	for (int i = 0; code(i) != NULL; i++) {
		Node *node = code(i);
		sema_toplevel(node);
	}
}
