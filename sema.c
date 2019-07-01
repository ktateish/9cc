// vim:set filetype=c tabstop=8 shiftwidth=8 noexpandtab:
#include <stdlib.h>

#include "9cc.h"

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
	node->type = v->type;
}

void sema_rec(Node *node) {
	if (node == NULL) return;

	Type *tp;
	if (node->kind == ND_DEFINE_INT_VAR) {
		if (var_duplicated(node->name)) {
			error_at(node->input, "duplicate definition");
		}
		var_put(node->name, node->type);
		return;
	}
	if (node->kind == ND_IDENT) {
		sema_ident(node);
		if (node->type->kind == TP_ARRAY) {
			Type *tp = result_type_enref(node->type->ptr_to);
			Node *nn = new_node(ND_ENREF, node_dup(node), NULL);
			*node = *nn;
			node->type = tp;
		}
		return;
	}
	if (node->kind == ND_BLOCK) {
		push_scope();
		for (int i = 0; i < vec_len(node->stmts); i++) {
			sema_rec(vec_at(node->stmts, i));
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
		Type *tp = v->type;
		if (tp->kind != TP_FUNCTION) {
			error("cannot call non-function type: %s", node->name);
		}
		for (int i = 0; i < vec_len(node->args); i++) {
			sema_rec(vec_at(node->args, i));
			Type *lhs = vec_at(tp->params, i);
			Node *nd = vec_at(node->args, i);
			Type *rhs = nd->type;
			if (!assignable(lhs, rhs)) {
				error(
				    "%s requires %s type for %d-th parameter "
				    "but %s type is given",
				    node->name, type_name(lhs), i + 1,
				    type_name(rhs));
			}
		}
		node->type = tp->returning;
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
		tp = result_type_enref(node->lhs->type);
		node->type = tp;
		return;
	}
	if (node->kind == ND_DEREF) {
		sema_rec(node->lhs);
		tp = result_type_deref(node->lhs->type);
		if (tp == NULL) {
			error("cannot derefer non pointer type");
		}
		node->type = tp;
		return;
	}
	if (node->kind == ND_RETURN) {
		sema_rec(node->lhs);
		tp = node->lhs->type;
		// XXX: check whether match the returning type of the
		// function
		if (tp == NULL || tp->kind == TP_UNDETERMINED) {
			error("cannot return undetermined type");
		}
		node->type = tp;
		return;
	}
	if (node->kind == ND_SIZEOF) {
		if (node->lhs->kind == ND_IDENT) {
			sema_ident(node->lhs);
		} else {
			sema_rec(node->lhs);
		}
		int sz = type_size(node->lhs->type);
		Node *sznd = new_node_num(sz);
		sema_rec(sznd);
		*node = *sznd;
		return;
	}
	if (node->kind == '=') {
		sema_rec(node->lhs);
		sema_rec(node->rhs);
		tp = result_type(node->kind, node->lhs->type, node->rhs->type);
		if (tp == NULL) {
			error("type unmatch: lhs: %s, rhs: %s",
			      type_name(node->lhs->type),
			      type_name(node->rhs->type));
		}
		node->type = tp;
		return;
	}
	if (node->kind == ND_NUM) {
		node->type = new_type_int();
		return;
	}
	{
		Node *lhs = node->lhs;
		Node *rhs = node->rhs;
		sema_rec(lhs);
		sema_rec(rhs);

		if (lhs->type->kind == TP_POINTER &&
		    rhs->type->kind == TP_INT) {
			int sz = type_size_refering(lhs->type);
			rhs = new_node('*', new_node_num(sz), rhs);
			rhs->type = new_type_int();
			rhs->lhs->type = new_type_int();
			node->rhs = rhs;
		}
		if (lhs->type->kind == TP_INT &&
		    rhs->type->kind == TP_POINTER) {
			int sz = type_size_refering(rhs->type);
			lhs = new_node('*', new_node_num(sz), lhs);
			lhs->type = new_type_int();
			lhs->lhs->type = new_type_int();
			node->lhs = lhs;
		}

		tp = result_type(node->kind, lhs->type, rhs->type);
		if (tp == NULL) {
			error("type unmatch: lhs: %s, rhs: %s",
			      type_name(lhs->type), type_name(rhs->type));
		}
		node->type = tp;
	}
}

void sema_toplevel(Node *node) {
	if (node->kind == ND_DECLARE_FUNC) {
		if (var_duplicated(node->name)) {
			error("duplicate definition: %s", node->name);
		}
		var_put(node->name, node->type);
	} else if (node->kind == ND_DEFINE_FUNC) {
		Var *declared = var_get(node->name);
		if (declared != NULL) {
			// XXX: check type
		}
		var_put(node->name, node->type);
		init_function_scope();
		for (int i = 0; i < vec_len(node->params); i++) {
			sema_rec(vec_at(node->params, i));
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
