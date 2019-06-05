// vim:set filetype=c tabstop=8 shiftwidth=8 noexpandtab:
#include <stdio.h>
#include <stdlib.h>

#include "9cc.h"

// Vector
Vector *new_vector() {
	Vector *vec = malloc(sizeof(Vector));
	vec->data = malloc(sizeof(void *) * 16);
	vec->capacity = 16;
	vec->len = 0;
	return vec;
}

void vec_push(Vector *vec, void *elem) {
	if (vec->capacity == vec->len) {
		vec->capacity *= 2;
		vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
	}
	vec->data[vec->len++] = elem;
}

// Test
void expect(int line, int expected, int actual) {
	if (expected == actual) return;
	fprintf(stderr, "%d: %d expected, but got %d\n", line, expected,
		actual);
	exit(1);
}

void runtest() {
	Vector *vec = new_vector();
	expect(__LINE__, 0, vec->len);

	for (long i = 0; i < 100; i++) vec_push(vec, (void *)i);

	expect(__LINE__, 100, vec->len);
	expect(__LINE__, 0, (long)vec->data[0]);
	expect(__LINE__, 50, (long)vec->data[50]);
	expect(__LINE__, 99, (long)vec->data[99]);

	printf("OK\n");
}
