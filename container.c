// vim:set filetype=c tabstop=8 shiftwidth=8 noexpandtab:
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

// Vector
struct Vector {
	void **data;
	int capacity;
	int len;
};

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

int vec_len(Vector *vec) { return vec->len; }
void *vec_at(Vector *vec, int i) { return vec->data[i]; }

char *string_join(Vector *strings, char *sep) {
	int sz = 1;
	for (int i = 0; i < vec_len(strings); i++) {
		char *s = vec_at(strings, i);
		sz += strlen(s);
	}
	sz += (vec_len(strings) - 1) * strlen(sep);

	char *s = malloc(sz);
	char *p = s;
	char *tep = "";
	for (int i = 0; i < vec_len(strings); i++) {
		char *e = strings->data[i];
		sprintf(p, "%s%s", tep, e);
		p += strlen(p);
		tep = sep;
	}
	return s;
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
	expect(__LINE__, 0, vec_len(vec));

	for (long i = 0; i < 100; i++) vec_push(vec, (void *)i);

	expect(__LINE__, 100, vec_len(vec));
	expect(__LINE__, 0, (long)vec_at(vec, 0));
	expect(__LINE__, 50, (long)vec_at(vec, 50));
	expect(__LINE__, 99, (long)vec_at(vec, 99));

	Vector *ss = new_vector();
	vec_push(ss, "foo");
	vec_push(ss, "bar");
	vec_push(ss, "buz");
	expect(__LINE__, 0, strcmp("foo, bar, buz", string_join(ss, ", ")));

	printf("OK\n");
}
