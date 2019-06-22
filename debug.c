// vim:set filetype=c tabstop=8 shiftwidth=8 noexpandtab:
#include <stdio.h>

#include "9cc.h"

void dump_token(Token *t) {
	fprintf(stderr, "--\n");
	fprintf(stderr, "Token: ");
	switch (t->ty) {
		case TK_IF:
			fprintf(stderr, "'if'\n");
			break;
		case TK_ELSE:
			fprintf(stderr, "'else'\n");
			break;
		case TK_WHILE:
			fprintf(stderr, "'for'\n");
			break;
		case TK_FOR:
			fprintf(stderr, "'for'\n");
			break;
		case TK_RETURN:
			fprintf(stderr, "'return'\n");
			break;
		case TK_IDENT:
			fprintf(stderr, "IDENTIFIER\n");
			fprintf(stderr, "Name: %s\n", t->name);
			break;
		case TK_EQ:
			fprintf(stderr, "'=='\n");
			break;
		case TK_NE:
			fprintf(stderr, "'!='\n");
			break;
		case TK_LE:
			fprintf(stderr, "'<='\n");
			break;
		case TK_GE:
			fprintf(stderr, "'>='\n");
			break;
		case TK_EOF:
			fprintf(stderr, "EOF\n");
			break;
		case TK_NUM:
			fprintf(stderr, "NUMBER\n");
			fprintf(stderr, "Value: %d\n", t->val);
			break;
		case TK_INT:
			fprintf(stderr, "INT\n");
			break;
		default:
			fprintf(stderr, "%c\n", t->ty);
	}
}

void dump_tokens() {
	for (int i = 0; tokens(i) != NULL; i++) {
		dump_token(tokens(i));
	}
}

void dump_node_rec(Node *node, int level);
void dump_vars(Var *vars, int level);
void dump_node(Node *node, int level) {
	fprintf(stderr, "%*s--\n", level * 2, "");
	fprintf(stderr, "%*sNode: ", level * 2, "");
	switch (node->ty) {
		case ND_DEREF:
			fprintf(stderr, "DEREF\n");
			fprintf(stderr, "%*sType: %s\n", level * 2, "",
				type_name(node->tp));
			dump_node_rec(node->lhs, level + 1);
			break;
		case ND_ENREF:
			fprintf(stderr, "ENREF\n");
			fprintf(stderr, "%*sType: %s\n", level * 2, "",
				type_name(node->tp));
			dump_node_rec(node->lhs, level + 1);
			break;
		case ND_DEFINE_FUNC:
			fprintf(stderr, "DEFINE_FUNC\n");
			fprintf(stderr, "%*sName: %s\n", level * 2, "",
				node->name);
			fprintf(stderr, "%*sType: %s\n", level * 2, "",
				type_name(node->tp));
			fprintf(stderr, "%*sParams:\n", level * 2, "");
			for (int i = 0; i < node->params->len; i++) {
				dump_node_rec(node->params->data[i], level + 1);
			}
			fprintf(stderr, "%*sVars:\n", level * 2, "");
			dump_vars(node->vars, level + 1);
			fprintf(stderr, "%*s--\n", level * 2, "");
			dump_node_rec(node->body, level + 1);
			break;
		case ND_DEFINE_INT_VAR:
			fprintf(stderr, "DEFINE_INT_VAR\n");
			fprintf(stderr, "%*sName: %s\n", level * 2, "",
				node->name);
			fprintf(stderr, "%*sType: %s\n", level * 2, "",
				type_name(node->tp));
			break;
		case ND_BLOCK:
			fprintf(stderr, "BLOCK\n");
			for (int i = 0; i < node->stmts->len; i++) {
				dump_node_rec(node->stmts->data[i], level + 1);
			}
			break;
		case ND_FUNCALL:
			fprintf(stderr, "FUNCALL\n");
			fprintf(stderr, "%*sName: %s\n", level * 2, "",
				node->name);
			break;
		case ND_IF:
			fprintf(stderr, "WHILE\n");
			dump_node_rec(node->cond, level + 1);
			dump_node_rec(node->thenc, level + 1);
			dump_node_rec(node->elsec, level + 1);
			break;
		case ND_WHILE:
			fprintf(stderr, "WHILE\n");
			dump_node_rec(node->cond, level + 1);
			dump_node_rec(node->body, level + 1);
			break;
		case ND_FOR:
			fprintf(stderr, "FOR\n");
			dump_node_rec(node->init, level + 1);
			dump_node_rec(node->cond, level + 1);
			dump_node_rec(node->update, level + 1);
			dump_node_rec(node->body, level + 1);
			break;
		case ND_RETURN:
			fprintf(stderr, "RETURN\n");
			fprintf(stderr, "%*sType: %s\n", level * 2, "",
				type_name(node->tp));
			dump_node_rec(node->lhs, level + 1);
			break;
		case ND_IDENT:
			fprintf(stderr, "IDENT\n");
			fprintf(stderr, "%*sName: %s\n", level * 2, "",
				node->name);
			fprintf(stderr, "%*sType: %s\n", level * 2, "",
				type_name(node->tp));
			break;
		case ND_EQ:
			fprintf(stderr, "'=='\n");
			fprintf(stderr, "%*sType: %s\n", level * 2, "",
				type_name(node->tp));
			dump_node_rec(node->lhs, level + 1);
			dump_node_rec(node->rhs, level + 1);
			break;
		case ND_NE:
			fprintf(stderr, "'!='\n");
			fprintf(stderr, "%*sType: %s\n", level * 2, "",
				type_name(node->tp));
			dump_node_rec(node->lhs, level + 1);
			dump_node_rec(node->rhs, level + 1);
			break;
		case ND_LE:
			fprintf(stderr, "'<='\n");
			fprintf(stderr, "%*sType: %s\n", level * 2, "",
				type_name(node->tp));
			dump_node_rec(node->lhs, level + 1);
			dump_node_rec(node->rhs, level + 1);
			break;
		case ND_NUM:
			fprintf(stderr, "NUMBER\n");
			fprintf(stderr, "%*sType: %s\n", level * 2, "",
				type_name(node->tp));
			fprintf(stderr, "%*sValue: %d\n", level * 2, "",
				node->val);
			break;
		default:
			fprintf(stderr, "%c\n", node->ty);
			fprintf(stderr, "%*sType: %s\n", level * 2, "",
				type_name(node->tp));
			dump_node_rec(node->lhs, level + 1);
			dump_node_rec(node->rhs, level + 1);
			break;
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

void dump_vars(Var *vars, int level) {
	for (Var *p = vars; p->next != NULL; p = p->next) {
		fprintf(stderr, "%*s--\n", level * 2, "");
		fprintf(stderr, "%*sVar: %s\n", level * 2, "", p->name);
		fprintf(stderr, "%*sType: %s\n", level * 2, "",
			type_name(p->tp));
		fprintf(stderr, "%*sOffset: %d\n", level * 2, "", p->offset);
	}
}
