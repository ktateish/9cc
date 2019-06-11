// vim:set filetype=c tabstop=8 shiftwidth=8 noexpandtab:
#include <stdio.h>

#include "9cc.h"

void gen_lval(Node *node) {
	if (node->ty != ND_IDENT) {
		error("assigning lvalue is not a variable");
	}

	int offset = var_offset(node->name);

	printf("  mov rax, rbp\n");
	printf("  sub rax, %d\n", offset);
	printf("  push rax\n");
}

int labelseq;

void gen(Node *node) {
	if (node->ty == ND_NUM) {
		printf("  push %d\n", node->val);
		return;
	}

	if (node->ty == ND_BLOCK) {
		for (int i = 0; i < node->stmts->len; i++) {
			gen(node->stmts->data[i]);
			printf("  pop rax\n");
		}
		return;
	}

	if (node->ty == ND_FUNCALL) {
		printf("  call %s\n", node->name);
		printf("  push rax\n");
		return;
	}

	if (node->ty == ND_IF) {
		int seq = labelseq++;
		if (node->elsec) {
			gen(node->cond);
			printf("  pop rax\n");
			printf("  push rax\n");
			printf("  cmp rax, 0\n");
			printf("  je .Lelse%d\n", seq);
			gen(node->thenc);
			printf("  jmp .Lend%d\n", seq);
			printf(".Lelse%d:\n", seq);
			gen(node->elsec);
			printf(".Lend%d:\n", seq);
		} else {
			gen(node->cond);
			printf("  pop rax\n");
			printf("  push rax\n");
			printf("  cmp rax, 0\n");
			printf("  je .Lend%d\n", seq);
			gen(node->thenc);
			printf(".Lend%d:\n", seq);
		}
		return;
	}

	if (node->ty == ND_WHILE) {
		int seq = labelseq++;
		printf(".Lbegin%d:\n", seq);
		gen(node->cond);
		printf("  pop rax\n");
		printf("  push rax\n");
		printf("  cmp rax, 0\n");
		printf("  je .Lend%d\n", seq);
		gen(node->body);
		printf("  jmp .Lbegin%d\n", seq);
		printf(".Lend%d:\n", seq);
		return;
	}

	if (node->ty == ND_FOR) {
		int seq = labelseq++;
		if (node->init) {
			gen(node->init);
		}
		printf(".Lbegin%d:\n", seq);
		if (node->cond) {
			gen(node->cond);
			printf("  pop rax\n");
			printf("  push rax\n");
			printf("  cmp rax, 0\n");
			printf("  je .Lend%d\n", seq);
		}
		gen(node->body);
		if (node->update) {
			gen(node->update);
		}
		printf("  jmp .Lbegin%d\n", seq);
		printf(".Lend%d:\n", seq);
		return;
	}

	if (node->ty == ND_RETURN) {
		gen(node->lhs);
		printf("  pop rax\n");
		printf("  mov rsp, rbp\n");
		printf("  pop rbp\n");
		printf("  ret\n");
		return;
	}

	if (node->ty == ND_IDENT) {
		gen_lval(node);
		printf("  pop rax\n");
		printf("  mov rax, [rax]\n");
		printf("  push rax\n");
		return;
	}

	if (node->ty == '=') {
		gen_lval(node->lhs);
		gen(node->rhs);

		printf("  pop rdi\n");
		printf("  pop rax\n");
		printf("  mov [rax], rdi\n");
		printf("  push rdi\n");
		return;
	}

	gen(node->lhs);
	gen(node->rhs);

	printf("  pop rdi\n");
	printf("  pop rax\n");

	switch (node->ty) {
		case '+':
			printf("  add rax, rdi\n");
			break;
		case '-':
			printf("  sub rax, rdi\n");
			break;
		case '*':
			printf("  imul rdi\n");
			break;
		case '/':
			printf("  cqo\n");
			printf("  idiv rdi\n");
			break;
		case ND_EQ:
			printf("  cmp rax, rdi\n");
			printf("  sete al\n");
			printf("  movzb rax, al\n");
			break;
		case ND_NE:
			printf("  cmp rax, rdi\n");
			printf("  setne al\n");
			printf("  movzb rax, al\n");
			break;
		case '<':
			printf("  cmp rax, rdi\n");
			printf("  setl al\n");
			printf("  movzb rax, al\n");
			break;
		case ND_LE:
			printf("  cmp rax, rdi\n");
			printf("  setle al\n");
			printf("  movzb rax, al\n");
			break;
	}
	printf("  push rax\n");
}
