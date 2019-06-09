// vim:set filetype=c tabstop=8 shiftwidth=8 noexpandtab:
#ifndef NINE_CC_H
#define NINE_CC_H

// Vector
typedef struct {
	void **data;
	int capacity;
	int len;
} Vector;

Vector *new_vector();
void vec_push(Vector *vec, void *elem);

//
// Tokenizer
//

// token types
enum TokenType {
	TK_NUM = 256,  // integer
	TK_IF,
	TK_RETURN,
	TK_IDENT,
	TK_EOF,
	TK_EQ,
	TK_NE,
	TK_LE,
	TK_GE,
};

// token
typedef struct {
	int ty;
	int val;
	char *name;
	char *input;
} Token;

void init_tokens();
void dump_tokens();

// function for reporting an error
void error(char *fmt, ...);

// function for reporting error location
void error_at(char *loc, char *msg);

// tokenize a string pointed by user_input and save them to tokens
void tokenize(char *p);

//
// Parser
//

// position for parser
extern int pos;

// node types
enum NodeType {
	ND_NUM = 256,
	ND_IF,
	ND_RETURN,
	ND_IDENT,
	ND_EQ,
	ND_NE,
	ND_LE,
};

typedef struct Node {
	int ty;  // operator or ND_NUM
	struct Node *lhs, *rhs, *cond, *thenc;
	int val;     // for ND_NUM
	char *name;  // for ND_IDENT
} Node;

void dump_nodes();
Node *code(int i);
void program();

// variables
typedef struct Var {
	struct Var *next;
	char *name;
	int offset;
} Var;

void var_put(char *name);
int var_offset(char *name);

//
// Code Generator
//
void gen(Node *node);

//
// Test
//
void runtest();

#endif  // NINE_CC_H
