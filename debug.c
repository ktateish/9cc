// vim:set filetype=c tabstop=8 shiftwidth=8 noexpandtab:
#include <stdio.h>

#include "9cc.h"

void dump_token(Token *t) {
	fprintf(stderr, "--\n");
	fprintf(stderr, "Token: ");
	if (t->kind == TK_IF)
		fprintf(stderr, "'if'\n");
	else if (t->kind == TK_ELSE)
		fprintf(stderr, "'else'\n");
	else if (t->kind == TK_WHILE)
		fprintf(stderr, "'for'\n");
	else if (t->kind == TK_FOR)
		fprintf(stderr, "'for'\n");
	else if (t->kind == TK_RETURN)
		fprintf(stderr, "'return'\n");
	else if (t->kind == TK_IDENT) {
		fprintf(stderr, "IDENTIFIER\n");
		fprintf(stderr, "Name: %s\n", t->name);
	} else if (t->kind == TK_EQ)
		fprintf(stderr, "'=='\n");
	else if (t->kind == TK_NE)
		fprintf(stderr, "'!='\n");
	else if (t->kind == TK_LE)
		fprintf(stderr, "'<='\n");
	else if (t->kind == TK_GE)
		fprintf(stderr, "'>='\n");
	else if (t->kind == TK_EOF)
		fprintf(stderr, "EOF\n");
	else if (t->kind == TK_NUM) {
		fprintf(stderr, "NUMBER\n");
		fprintf(stderr, "Value: %d\n", t->val);
	} else if (t->kind == TK_INT)
		fprintf(stderr, "INT\n");
	else
		fprintf(stderr, "%c\n", t->kind);
}

void dump_tokens() {
	for (int i = 0; tokens(i) != NULL; i++) {
		dump_token(tokens(i));
	}
}

void dump_node_rec(Node *node, int level);
void dump_scope(Scope *scope, int level);
void dump_node(Node *node, int level) {
	fprintf(stderr, "%*s--\n", level * 2, "");
	fprintf(stderr, "%*sNode: ", level * 2, "");
	if (node->kind == ND_DEREF) {
		fprintf(stderr, "DEREF\n");
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->type));
		dump_node_rec(node->lhs, level + 1);
	} else if (node->kind == ND_ENREF) {
		fprintf(stderr, "ENREF\n");
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->type));
		dump_node_rec(node->lhs, level + 1);
	} else if (node->kind == ND_DECLARE_FUNC) {
		fprintf(stderr, "DECLARE_FUNC\n");
		fprintf(stderr, "%*sName: %s\n", level * 2, "", node->name);
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->type));
	} else if (node->kind == ND_DEFINE_FUNC) {
		fprintf(stderr, "DEFINE_FUNC\n");
		fprintf(stderr, "%*sName: %s\n", level * 2, "", node->name);
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->type));
		fprintf(stderr, "%*sParams:\n", level * 2, "");
		for (int i = 0; i < vec_len(node->params); i++) {
			dump_node_rec(vec_at(node->params, i), level + 1);
		}
		fprintf(stderr, "%*sVars:\n", level * 2, "");
		dump_scope(node->scope, level + 1);
		fprintf(stderr, "%*s--\n", level * 2, "");
		dump_node_rec(node->body, level + 1);
	} else if (node->kind == ND_DEFINE_INT_VAR) {
		fprintf(stderr, "DEFINE_INT_VAR\n");
		fprintf(stderr, "%*sName: %s\n", level * 2, "", node->name);
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->type));
	} else if (node->kind == ND_BLOCK) {
		fprintf(stderr, "BLOCK\n");
		fprintf(stderr, "%*sVars:\n", level * 2, "");
		dump_scope(node->scope, level + 1);
		for (int i = 0; i < vec_len(node->stmts); i++) {
			dump_node_rec(vec_at(node->stmts, i), level + 1);
		}
	} else if (node->kind == ND_FUNCALL) {
		fprintf(stderr, "FUNCALL\n");
		fprintf(stderr, "%*sName: %s\n", level * 2, "", node->name);
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->type));
		fprintf(stderr, "%*sArgs:\n", level * 2, "");
		for (int i = 0; i < vec_len(node->args); i++) {
			dump_node_rec(vec_at(node->args, i), level + 1);
		}
	} else if (node->kind == ND_IF) {
		fprintf(stderr, "WHILE\n");
		dump_node_rec(node->cond, level + 1);
		dump_node_rec(node->thenc, level + 1);
		dump_node_rec(node->elsec, level + 1);
	} else if (node->kind == ND_WHILE) {
		fprintf(stderr, "WHILE\n");
		dump_node_rec(node->cond, level + 1);
		dump_node_rec(node->body, level + 1);
	} else if (node->kind == ND_FOR) {
		fprintf(stderr, "FOR\n");
		dump_node_rec(node->init, level + 1);
		dump_node_rec(node->cond, level + 1);
		dump_node_rec(node->update, level + 1);
		dump_node_rec(node->body, level + 1);
	} else if (node->kind == ND_RETURN) {
		fprintf(stderr, "RETURN\n");
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->type));
		dump_node_rec(node->lhs, level + 1);
	} else if (node->kind == ND_SIZEOF) {
		fprintf(stderr, "SIZEOF\n");
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->type));
		dump_node_rec(node->lhs, level + 1);
	} else if (node->kind == ND_IDENT) {
		fprintf(stderr, "IDENT\n");
		fprintf(stderr, "%*sName: %s\n", level * 2, "", node->name);
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->type));
	} else if (node->kind == ND_EQ) {
		fprintf(stderr, "'=='\n");
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->type));
		dump_node_rec(node->lhs, level + 1);
		dump_node_rec(node->rhs, level + 1);
	} else if (node->kind == ND_NE) {
		fprintf(stderr, "'!='\n");
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->type));
		dump_node_rec(node->lhs, level + 1);
		dump_node_rec(node->rhs, level + 1);
	} else if (node->kind == ND_LE) {
		fprintf(stderr, "'<='\n");
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->type));
		dump_node_rec(node->lhs, level + 1);
		dump_node_rec(node->rhs, level + 1);
	} else if (node->kind == ND_NUM) {
		fprintf(stderr, "NUMBER\n");
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->type));
		fprintf(stderr, "%*sValue: %d\n", level * 2, "", node->val);
	} else {
		fprintf(stderr, "%c\n", node->kind);
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->type));
		dump_node_rec(node->lhs, level + 1);
		dump_node_rec(node->rhs, level + 1);
	}
}

void dump_node_rec(Node *node, int level) {
	if (node == NULL) {
		return;
	}
	dump_node(node, level);
}

void dump_nodes() {
	for (int i = 0; code(i) != NULL; i++) {
		dump_node_rec(code(i), 0);
	}
}

void dump_scope(Scope *scope, int level) {
	for (Scope *s = scope; s != NULL; s = s->next) {
		for (Var *p = s->vars; p->next != NULL; p = p->next) {
			fprintf(stderr, "%*s--\n", level * 2, "");
			fprintf(stderr, "%*sVar: %s\n", level * 2, "", p->name);
			fprintf(stderr, "%*sType: %s\n", level * 2, "",
				type_name(p->type));
			fprintf(stderr, "%*sOffset: %d\n", level * 2, "",
				p->offset);
		}
		level = level + 1;
	}
}
