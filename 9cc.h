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

// token types
enum TokenType {
	TK_NUM = 256,  // integer
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
	char *input;
} Token;

Token *new_token(int ty, char *input);
Token *new_token_num(int val, char *input);

// input
extern char *user_input;

// tokenized results
extern Vector *token_vec;

void init_tokens();
void push_token(Token *t);
Token *tokens(int i);

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
	ND_EQ,
	ND_NE,
	ND_LE,
};

typedef struct Node {
	int ty;  // operator or ND_NUM
	struct Node *lhs, *rhs;
	int val;  // for ND_NUM
} Node;

Node *new_node(int node_type, Node *lhs, Node *rhs);
Node *new_node_num(int val);

int consume(int ty);

Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *term();
Node *expr();

void gen(Node *node);

// Test
void expect(int line, int expected, int actual);

void runtest();

#endif  // NINE_CC_H
