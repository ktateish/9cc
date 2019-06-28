// vim:set filetype=c tabstop=8 shiftwidth=8 noexpandtab:
#include <stdio.h>
#include <stdlib.h>

#include "9cc.h"

int labelseq;
void gen(Node *node);

void gen_lval(Node *node) {
	if (node->kind != ND_IDENT) {
		error("assigning lvalue is not a variable");
	}

	Var *v = var_get(node->name);
	if (v == NULL) {
		error("unknown variable: %s", node->name);
	}
	int offset = v->offset;

	printf("  mov rax, rbp\n");
	printf("  sub rax, %d\n", offset);
	printf("  push rax\n");
}

void gen_define_func(Node *node) {
	if (node->kind != ND_DEFINE_FUNC) {
		error("not function definition");
	}
	printf("%s:\n", node->name);
	var_use(node);
	// Prologue
	printf("  push rbp\n");
	printf("  mov rbp, rsp\n");
	printf("  sub rsp, %d\n", node->max_offset);
	for (int i = 0; i < node->params->len; i++) {
		if (i == 5)
			printf("  mov [rbp-48], r9\n");
		else if (i == 4)
			printf("  mov [rbp-40], r8\n");
		else if (i == 3)
			printf("  mov [rbp-32], rcx\n");
		else if (i == 2)
			printf("  mov [rbp-24], rdx\n");
		else if (i == 1)
			printf("  mov [rbp-16], rsi\n");
		else if (i == 0)
			printf("  mov [rbp-8], rdi\n");
		else {
			fprintf(stderr,
				"function parameters more than "
				"6 is not supported\n");
			exit(1);
		}
	}
	gen(node->body);
}

void gen_define_int_var(Node *node) {
	printf("  push 1000000007\n");
	return;
}

void gen_funcall(Node *node) {
	if (node->kind != ND_FUNCALL) {
		error("not function call");
	}
	int n = node->args->len;
	// Check whther rsp is 16byte-aligned
	//
	// a) if algined:
	//     |   ...  |
	//     |    x   | <- original stack top (aligned)
	//     |    0   | <- padding (not aligned)
	//     |    1   | <- pop flag (aligned)
	//
	// b) if not algined:
	//     |   ...  |
	//     |    x   | <- original stack top (not aligned)
	//     |    0   | <- pop flag (aligned)
	//
	printf("  mov rax, rsp\n");
	printf("  mov rdi, 16\n");
	printf("  cqo\n");
	printf("  idiv rdi\n");
	printf("  mov rax, 0\n");  // pop flag
	printf("  cmp rdi, 0\n");
	int seq = labelseq++;
	printf("  jne .Lprecall%d\n", seq);
	// if aligned, push a '0' as padding and 1 as pop flag
	printf("  push 0\n");
	printf("  mov rax, 1\n");
	printf(".Lprecall%d:\n", seq);
	// if not aligned, push 0 as pop flag
	printf("  push rax\n");

	for (int i = n - 1; 0 <= i; i--) {
		gen(node->args->data[i]);
		if (i == 5)
			printf("  pop r9\n");
		else if (i == 4)
			printf("  pop r8\n");
		else if (i == 3)
			printf("  pop rcx\n");
		else if (i == 2)
			printf("  pop rdx\n");
		else if (i == 1)
			printf("  pop rsi\n");
		else if (i == 0)
			printf("  pop rdi\n");
	}

	printf("  call %s\n", node->name);
	// if pop flag is true, remove padding 0.
	printf("  pop rdi\n");  // pop flag
	printf("  cmp rdi, 0\n");
	printf("  je .Lpostcall%d\n", seq);
	printf("  pop rdi\n");
	printf(".Lpostcall%d:\n", seq);
	printf("  push rax\n");
}

void gen_if(Node *node) {
	if (node->kind != ND_IF) {
		error("not if syntax");
	}
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
}

void gen_while(Node *node) {
	if (node->kind != ND_WHILE) {
		error("not while syntax");
	}
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
}

void gen_for(Node *node) {
	if (node->kind != ND_FOR) {
		error("not for syntax");
	}
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
}

void gen_return(Node *node) {
	if (node->kind != ND_RETURN) {
		error("not return syntax");
	}
	gen(node->lhs);
	printf("  pop rax\n");
	printf("  mov rsp, rbp\n");
	printf("  pop rbp\n");
	printf("  ret\n");
}

void gen_ident(Node *node) {
	if (node->kind != ND_IDENT) {
		error("not identifier");
	}
	gen_lval(node);
	printf("  pop rax\n");
	printf("  mov rax, [rax]\n");
	printf("  push rax\n");
}

void gen_deref(Node *node) {
	if (node->kind != ND_DEREF) {
		error("not identifier");
	}
	gen(node->lhs);
	printf("  pop rax\n");
	printf("  mov rax, [rax]\n");
	printf("  push rax\n");
}

void gen_enref(Node *node) {
	if (node->kind != ND_ENREF) {
		error("not identifier");
	}
	node = node->lhs;
	if (node->kind != ND_IDENT) {
		error("not identifier");
	}
	gen_lval(node);
}

void gen_assign(Node *node) {
	if (node->kind != '=') {
		error("not assign syntax");
	}
	Node *lhs = node->lhs;
	if (lhs->kind == ND_IDENT) {
		gen_lval(node->lhs);
	} else if (lhs->kind == ND_DEREF) {
		gen(lhs->lhs);
	}
	gen(node->rhs);

	printf("  pop rdi\n");
	printf("  pop rax\n");
	printf("  mov [rax], rdi\n");
	printf("  push rdi\n");
}

void gen_binary_operator(Node *node) {
	gen(node->lhs);
	gen(node->rhs);

	printf("  pop rdi\n");
	printf("  pop rax\n");

	if (node->kind == '+') {
		printf("  add rax, rdi\n");
	} else if (node->kind == '-') {
		printf("  sub rax, rdi\n");
	} else if (node->kind == '*') {
		printf("  imul rdi\n");
	} else if (node->kind == '/') {
		printf("  cqo\n");
		printf("  idiv rdi\n");
	} else if (node->kind == ND_EQ) {
		printf("  cmp rax, rdi\n");
		printf("  sete al\n");
		printf("  movzb rax, al\n");
	} else if (node->kind == ND_NE) {
		printf("  cmp rax, rdi\n");
		printf("  setne al\n");
		printf("  movzb rax, al\n");
	} else if (node->kind == '<') {
		printf("  cmp rax, rdi\n");
		printf("  setl al\n");
		printf("  movzb rax, al\n");
	} else if (node->kind == ND_LE) {
		printf("  cmp rax, rdi\n");
		printf("  setle al\n");
		printf("  movzb rax, al\n");
	} else {
		error("unknown binary operator: %c", node->kind);
	}
	printf("  push rax\n");
}

void gen(Node *node) {
	Scope *saved;
	if (node->kind == ND_NUM) {
		printf("  push %d\n", node->val);
	} else if (node->kind == ND_DECLARE_FUNC) {
		;
	} else if (node->kind == ND_DEFINE_FUNC) {
		gen_define_func(node);
	} else if (node->kind == ND_DEFINE_INT_VAR) {
		gen_define_int_var(node);
	} else if (node->kind == ND_BLOCK) {
		saved = scope_use(node->scope);
		for (int i = 0; i < node->stmts->len; i++) {
			gen(node->stmts->data[i]);
			if (i + 1 < node->stmts->len) {
				printf("  pop rax\n");
			}
		}
		scope_use(saved);
	} else if (node->kind == ND_FUNCALL) {
		gen_funcall(node);
	} else if (node->kind == ND_IF) {
		gen_if(node);
	} else if (node->kind == ND_WHILE) {
		gen_while(node);
	} else if (node->kind == ND_FOR) {
		gen_for(node);
	} else if (node->kind == ND_RETURN) {
		gen_return(node);
	} else if (node->kind == ND_IDENT) {
		gen_ident(node);
	} else if (node->kind == ND_DEREF) {
		gen_deref(node);
	} else if (node->kind == ND_ENREF) {
		gen_enref(node);
	} else if (node->kind == '=') {
		gen_assign(node);
	} else {
		gen_binary_operator(node);
	}
}
