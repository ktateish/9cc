// vim:set filetype=c tabstop=8 shiftwidth=8 noexpandtab:
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

int TypeSize[] = {
    0,  // TP_UNDETERMINED,
    4,  // TP_INT,
    8,  // TP_POINTER,
    8,  // TP_FUNCTION,
};

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

Type *result_type_add(Type *lhs, Type *rhs) {
	if (lhs->kind == TP_INT && rhs->kind == TP_INT) return lhs;
	if (lhs->kind == TP_POINTER && rhs->kind == TP_INT) return lhs;
	if (lhs->kind == TP_INT && rhs->kind == TP_POINTER) return rhs;
	if (lhs->kind == TP_POINTER && rhs->kind == TP_POINTER) return NULL;
	return NULL;
}

Type *result_type_sub(Type *lhs, Type *rhs) {
	if (lhs->kind == TP_INT && rhs->kind == TP_INT) return lhs;
	if (lhs->kind == TP_POINTER && rhs->kind == TP_INT) return lhs;
	if (lhs->kind == TP_INT && rhs->kind == TP_POINTER) return rhs;
	if (lhs->kind == TP_POINTER && rhs->kind == TP_POINTER) return rhs;
	return NULL;
}

Type *result_type_mul(Type *lhs, Type *rhs) {
	if (lhs->kind == TP_INT && rhs->kind == TP_INT) return lhs;
	return NULL;
}

Type *result_type_div(Type *lhs, Type *rhs) {
	if (lhs->kind == TP_INT && rhs->kind == TP_INT) return lhs;
	return NULL;
}

int assignable(Type *lhs, Type *rhs) {
	if (lhs->kind == TP_POINTER && rhs->kind == TP_POINTER) {
		return assignable(lhs->ptr_to, rhs->ptr_to);
	}
	return lhs->kind == rhs->kind;
}

Type *result_type_assign(Type *lhs, Type *rhs) {
	if (assignable(lhs, rhs)) return lhs;
	return NULL;
}

Type *result_type_eq(Type *lhs, Type *rhs) {
	if (lhs->kind == TP_INT && rhs->kind == TP_INT) return lhs;
	return NULL;
}

Type *result_type_enref(Type *tp) { return new_type_ptr(tp); }

Type *result_type_deref(Type *tp) {
	if (tp->kind != TP_POINTER) {
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

void sema_ident(Node *node) {
	Var *v = var_get(node->name);
	if (v == NULL) {
		error_at(node->input, "unknown variable");
	}
	node->tp = v->tp;
}

void sema_rec(Node *node) {
	if (node == NULL) return;

	Type *tp;
	if (node->kind == ND_DEFINE_INT_VAR) {
		if (var_duplicated(node->name)) {
			error_at(node->input, "duplicate definition");
		}
		var_put(node->name, node->tp);
		return;
	}
	if (node->kind == ND_IDENT) {
		sema_ident(node);
		if (node->tp->kind == TP_ARRAY) {
			Type *tp = result_type_enref(node->tp->ptr_to);
			Node *nn = new_node(ND_ENREF, node_dup(node), NULL);
			*node = *nn;
			node->tp = tp;
		}
		return;
	}
	if (node->kind == ND_BLOCK) {
		push_scope();
		for (int i = 0; i < node->stmts->len; i++) {
			sema_rec(node->stmts->data[i]);
		}
		set_scope(node);
		pop_scope();
		return;
	}
	if (node->kind == ND_FUNCALL) {
		Var *v = var_get(node->name);
		if (v == NULL) {
			error("undefined function: %s", node->name);
		}
		Type *tp = v->tp;
		if (tp->kind != TP_FUNCTION) {
			error("cannot call non-function type: %s", node->name);
		}
		for (int i = 0; i < node->args->len; i++) {
			sema_rec(node->args->data[i]);
			Type *lhs = tp->params->data[i];
			Node *nd = node->args->data[i];
			Type *rhs = nd->tp;
			if (!assignable(lhs, rhs)) {
				error(
				    "%s requires %s type for %d-th parameter "
				    "but %s type is given",
				    node->name, type_name(lhs), i + 1,
				    type_name(rhs));
			}
		}
		node->tp = tp->returning;
		return;
	}
	if (node->kind == ND_IF) {
		sema_rec(node->cond);
		sema_rec(node->thenc);
		sema_rec(node->elsec);
		return;
	}
	if (node->kind == ND_WHILE) {
		sema_rec(node->cond);
		sema_rec(node->body);
		return;
	}
	if (node->kind == ND_FOR) {
		sema_rec(node->init);
		sema_rec(node->cond);
		sema_rec(node->update);
		sema_rec(node->body);
		return;
	}
	if (node->kind == ND_ENREF) {
		// XXX: add the case for arrays
		sema_rec(node->lhs);
		if (node->lhs->kind != ND_IDENT) {
			error("cannot get address of non-identifier");
		}
		tp = result_type_enref(node->lhs->tp);
		node->tp = tp;
		return;
	}
	if (node->kind == ND_DEREF) {
		sema_rec(node->lhs);
		tp = result_type_deref(node->lhs->tp);
		if (tp == NULL) {
			error("cannot derefer non pointer type");
		}
		node->tp = tp;
		return;
	}
	if (node->kind == ND_RETURN) {
		sema_rec(node->lhs);
		tp = node->lhs->tp;
		// XXX: check whether match the returning type of the
		// function
		if (tp == NULL || tp->kind == TP_UNDETERMINED) {
			error("cannot return undetermined type");
		}
		node->tp = tp;
		return;
	}
	if (node->kind == ND_SIZEOF) {
		if (node->lhs->kind == ND_IDENT) {
			sema_ident(node->lhs);
		} else {
			sema_rec(node->lhs);
		}
		int sz = TypeSize[node->lhs->tp->kind];
		if (node->lhs->tp->kind == TP_ARRAY) {
			sz = TypeSize[node->lhs->tp->ptr_to->kind] *
			     node->lhs->tp->array_size;
		}
		Node *sznd = new_node_num(sz);
		sema_rec(sznd);
		*node = *sznd;
		return;
	}
	if (node->kind == '=') {
		sema_rec(node->lhs);
		sema_rec(node->rhs);
		tp = result_type(node->kind, node->lhs->tp, node->rhs->tp);
		if (tp == NULL) {
			error("type unmatch: lhs: %s, rhs: %s",
			      type_name(node->lhs->tp),
			      type_name(node->rhs->tp));
		}
		node->tp = tp;
		return;
	}
	if (node->kind == ND_NUM) {
		node->tp = new_type_int();
		return;
	}
	{
		Node *lhs = node->lhs;
		Node *rhs = node->rhs;
		sema_rec(lhs);
		sema_rec(rhs);

		if (lhs->tp->kind == TP_POINTER && rhs->tp->kind == TP_INT) {
			int sz = TypeSize[lhs->tp->ptr_to->kind];
			rhs = new_node('*', new_node_num(sz), rhs);
			rhs->tp = new_type_int();
			rhs->lhs->tp = new_type_int();
			node->rhs = rhs;
		}
		if (lhs->tp->kind == TP_INT && rhs->tp->kind == TP_POINTER) {
			int sz = TypeSize[rhs->tp->ptr_to->kind];
			lhs = new_node('*', new_node_num(sz), lhs);
			lhs->tp = new_type_int();
			lhs->lhs->tp = new_type_int();
			node->lhs = lhs;
		}

		tp = result_type(node->kind, lhs->tp, rhs->tp);
		if (tp == NULL) {
			error("type unmatch: lhs: %s, rhs: %s",
			      type_name(lhs->tp), type_name(rhs->tp));
		}
		node->tp = tp;
	}
}

void sema_toplevel(Node *node) {
	if (node->kind == ND_DECLARE_FUNC) {
		if (var_duplicated(node->name)) {
			error("duplicate definition: %s", node->name);
		}
		var_put(node->name, node->tp);
	} else if (node->kind == ND_DEFINE_FUNC) {
		Var *declared = var_get(node->name);
		if (declared != NULL) {
			// XXX: check type
		}
		var_put(node->name, node->tp);
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
