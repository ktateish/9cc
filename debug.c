// vim:set filetype=c tabstop=8 shiftwidth=8 noexpandtab:
#include <stdio.h>

#include "9cc.h"

void dump_token(Token *t) {
	fprintf(stderr, "--\n");
	fprintf(stderr, "Token: ");
	if (t->ty == TK_IF)
		fprintf(stderr, "'if'\n");
	else if (t->ty == TK_ELSE)
		fprintf(stderr, "'else'\n");
	else if (t->ty == TK_WHILE)
		fprintf(stderr, "'for'\n");
	else if (t->ty == TK_FOR)
		fprintf(stderr, "'for'\n");
	else if (t->ty == TK_RETURN)
		fprintf(stderr, "'return'\n");
	else if (t->ty == TK_IDENT) {
		fprintf(stderr, "IDENTIFIER\n");
		fprintf(stderr, "Name: %s\n", t->name);
	} else if (t->ty == TK_EQ)
		fprintf(stderr, "'=='\n");
	else if (t->ty == TK_NE)
		fprintf(stderr, "'!='\n");
	else if (t->ty == TK_LE)
		fprintf(stderr, "'<='\n");
	else if (t->ty == TK_GE)
		fprintf(stderr, "'>='\n");
	else if (t->ty == TK_EOF)
		fprintf(stderr, "EOF\n");
	else if (t->ty == TK_NUM) {
		fprintf(stderr, "NUMBER\n");
		fprintf(stderr, "Value: %d\n", t->val);
	} else if (t->ty == TK_INT)
		fprintf(stderr, "INT\n");
	else
		fprintf(stderr, "%c\n", t->ty);
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
	if (node->ty == ND_DEREF) {
		fprintf(stderr, "DEREF\n");
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->tp));
		dump_node_rec(node->lhs, level + 1);
	} else if (node->ty == ND_ENREF) {
		fprintf(stderr, "ENREF\n");
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->tp));
		dump_node_rec(node->lhs, level + 1);
	} else if (node->ty == ND_DECLARE_FUNC) {
		fprintf(stderr, "DECLARE_FUNC\n");
		fprintf(stderr, "%*sName: %s\n", level * 2, "", node->name);
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->tp));
	} else if (node->ty == ND_DEFINE_FUNC) {
		fprintf(stderr, "DEFINE_FUNC\n");
		fprintf(stderr, "%*sName: %s\n", level * 2, "", node->name);
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->tp));
		fprintf(stderr, "%*sParams:\n", level * 2, "");
		for (int i = 0; i < node->params->len; i++) {
			dump_node_rec(node->params->data[i], level + 1);
		}
		fprintf(stderr, "%*sVars:\n", level * 2, "");
		dump_scope(node->scope, level + 1);
		fprintf(stderr, "%*s--\n", level * 2, "");
		dump_node_rec(node->body, level + 1);
	} else if (node->ty == ND_DEFINE_INT_VAR) {
		fprintf(stderr, "DEFINE_INT_VAR\n");
		fprintf(stderr, "%*sName: %s\n", level * 2, "", node->name);
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->tp));
	} else if (node->ty == ND_BLOCK) {
		fprintf(stderr, "BLOCK\n");
		fprintf(stderr, "%*sVars:\n", level * 2, "");
		dump_scope(node->scope, level + 1);
		for (int i = 0; i < node->stmts->len; i++) {
			dump_node_rec(node->stmts->data[i], level + 1);
		}
	} else if (node->ty == ND_FUNCALL) {
		fprintf(stderr, "FUNCALL\n");
		fprintf(stderr, "%*sName: %s\n", level * 2, "", node->name);
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->tp));
		fprintf(stderr, "%*sArgs:\n", level * 2, "");
		for (int i = 0; i < node->args->len; i++) {
			dump_node_rec(node->args->data[i], level + 1);
		}
	} else if (node->ty == ND_IF) {
		fprintf(stderr, "WHILE\n");
		dump_node_rec(node->cond, level + 1);
		dump_node_rec(node->thenc, level + 1);
		dump_node_rec(node->elsec, level + 1);
	} else if (node->ty == ND_WHILE) {
		fprintf(stderr, "WHILE\n");
		dump_node_rec(node->cond, level + 1);
		dump_node_rec(node->body, level + 1);
	} else if (node->ty == ND_FOR) {
		fprintf(stderr, "FOR\n");
		dump_node_rec(node->init, level + 1);
		dump_node_rec(node->cond, level + 1);
		dump_node_rec(node->update, level + 1);
		dump_node_rec(node->body, level + 1);
	} else if (node->ty == ND_RETURN) {
		fprintf(stderr, "RETURN\n");
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->tp));
		dump_node_rec(node->lhs, level + 1);
	} else if (node->ty == ND_IDENT) {
		fprintf(stderr, "IDENT\n");
		fprintf(stderr, "%*sName: %s\n", level * 2, "", node->name);
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->tp));
	} else if (node->ty == ND_EQ) {
		fprintf(stderr, "'=='\n");
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->tp));
		dump_node_rec(node->lhs, level + 1);
		dump_node_rec(node->rhs, level + 1);
	} else if (node->ty == ND_NE) {
		fprintf(stderr, "'!='\n");
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->tp));
		dump_node_rec(node->lhs, level + 1);
		dump_node_rec(node->rhs, level + 1);
	} else if (node->ty == ND_LE) {
		fprintf(stderr, "'<='\n");
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->tp));
		dump_node_rec(node->lhs, level + 1);
		dump_node_rec(node->rhs, level + 1);
	} else if (node->ty == ND_NUM) {
		fprintf(stderr, "NUMBER\n");
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->tp));
		fprintf(stderr, "%*sValue: %d\n", level * 2, "", node->val);
	} else {
		fprintf(stderr, "%c\n", node->ty);
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(node->tp));
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
				type_name(p->tp));
			fprintf(stderr, "%*sOffset: %d\n", level * 2, "",
				p->offset);
		}
		level = level + 1;
	}
}
