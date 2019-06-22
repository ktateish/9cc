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
	switch (op) {
		case '+':
			return result_type_add(lhs, rhs);
		case '-':
			return result_type_sub(lhs, rhs);
		case '*':
			return result_type_mul(lhs, rhs);
		case '/':
			return result_type_div(lhs, rhs);
		case '=':
			return result_type_assign(lhs, rhs);
		case ND_EQ:
		case ND_NE:
		case ND_LE:
		case '<':
			return result_type_eq(lhs, rhs);
		case ND_ENREF:
			return result_type_enref(lhs);
		case ND_DEREF:
			return result_type_deref(lhs);
		default:
			return NULL;
	}
}

void sema_rec(Node *node) {
	if (node == NULL) return;

	Var *v;
	Type *tp;
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
			node->tp = new_type_int();
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
		case ND_ENREF:
			sema_rec(node->lhs);
			if (node->lhs->ty != ND_IDENT) {
				error("cannot get address of non-identifier");
			}
			tp = result_type_enref(node->lhs->tp);
			node->tp = tp;
			return;
		case ND_DEREF:
			sema_rec(node->lhs);
			tp = result_type_deref(node->lhs->tp);
			if (tp == NULL) {
				error("cannot derefer non pointer type");
			}
			node->tp = tp;
			return;
		case ND_RETURN:
			sema_rec(node->lhs);
			tp = node->lhs->tp;
			// XXX: check whether match the returning type of the
			// function
			if (tp == NULL || tp->ty == TP_UNDETERMINED) {
				error("cannot return undetermined type");
			}
			node->tp = tp;
			return;
		case '=':
			sema_rec(node->lhs);
			sema_rec(node->rhs);
			tp =
			    result_type(node->ty, node->lhs->tp, node->rhs->tp);
			if (tp == NULL) {
				error("type unmatch: lhs: %s, rhs: %s",
				      type_name(node->lhs->tp),
				      type_name(node->rhs->tp));
			}
			node->tp = tp;
			return;
		case ND_NUM:
			node->tp = new_type_int();
			return;
		case ND_EQ:
		case ND_NE:
		case ND_LE:
		default:
			sema_rec(node->lhs);
			sema_rec(node->rhs);
			tp =
			    result_type(node->ty, node->lhs->tp, node->rhs->tp);
			if (tp == NULL) {
				error("type unmatch: lhs: %s, rhs: %s",
				      type_name(node->lhs->tp),
				      type_name(node->rhs->tp));
			}
			node->tp = tp;
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
