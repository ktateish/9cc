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

// input
extern char *user_input;

// token types
enum TokenType {
	TK_NUM = 256,  // integer
	TK_IF,
	TK_ELSE,
	TK_WHILE,
	TK_FOR,
	TK_RETURN,
	TK_IDENT,
	TK_EOF,
	TK_EQ,
	TK_NE,
	TK_LE,
	TK_GE,
	TK_INT,
};

// token
typedef struct {
	int ty;
	int val;
	char *name;
	char *input;
} Token;

void init_tokens();
Token *tokens(int i);
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

// variables
typedef struct Var {
	struct Var *next;
	char *name;
	int offset;
} Var;

void var_use(Var *vars);
int var_offset(char *name);

// node types
enum NodeType {
	ND_NUM = 256,
	ND_DEFINE_FUNC,
	ND_DEFINE_INT_VAR,
	ND_BLOCK,
	ND_FUNCALL,
	ND_IF,
	ND_ELSE,
	ND_WHILE,
	ND_FOR,
	ND_RETURN,
	ND_IDENT,
	ND_EQ,
	ND_NE,
	ND_LE,
};

typedef struct Node {
	int ty;  // node type

	struct Node *lhs, *rhs;      // for binary/unary operators
	struct Node *cond;	   // for ND_IF, ND_WHILE, ND_FOR
	struct Node *thenc, *elsec;  // for ND_IF syntax
	struct Node *init, *update;  // for ND_FOR syntax
	struct Node *body;	   // for ND_DEFINE_FUNC, ND_WHILE, ND_FOR

	int val;  // for ND_NUM

	char *name;  // for ND_IDENT, ND_DEFINE_FUNC

	Vector *stmts;  // for ND_BLOCK
	Vector *args;   // for ND_FUNCALL

	Vector *params;  // for ND_DEFINE_FUNC
	Var *vars;       // for ND_DEFINE_FUNC

	char *input;  // for ND_DEFINE_INT_VAR, ND_IDENT
} Node;

void dump_nodes();
Node *code(int i);
void program();

//
// Semantic Analysis
//
void sema();

//
// Code Generator
//
void gen(Node *node);

//
// Test
//
void runtest();

#endif  // NINE_CC_H
