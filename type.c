// vim:set filetype=c tabstop=8 shiftwidth=8 noexpandtab:
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

// Type types
Type *new_type_int() {
	Type *tp = malloc(sizeof(Type));
	tp->kind = TP_INT;
	return tp;
}

Type *new_type_ptr(Type *ptr_to) {
	Type *tp = malloc(sizeof(Type));
	tp->kind = TP_POINTER;
	tp->ptr_to = ptr_to;
	return tp;
}

Type *new_type_function(Vector *params, Type *returning) {
	Type *tp = malloc(sizeof(Type));
	tp->kind = TP_FUNCTION;
	tp->params = params;
	tp->returning = returning;
	return tp;
}

Type *new_type_array(Type *ptr_to, int array_size) {
	Type *tp = malloc(sizeof(Type));
	tp->kind = TP_ARRAY;
	tp->ptr_to = ptr_to;
	tp->array_size = array_size;
	return tp;
}

Type *new_type_undetermined() {
	Type *tp = malloc(sizeof(TP_UNDETERMINED));
	tp->kind = TP_UNDETERMINED;
	return tp;
}

char *type_name(Type *tp) {
	if (tp == NULL) return "(null)";
	if (tp->kind == TP_INT) return "int";
	if (tp->kind == TP_POINTER) {
		char *src = type_name(tp->ptr_to);
		char *s = malloc(strlen(src) + 2);
		sprintf(s, "*%s", src);
		return s;
	}
	if (tp->kind == TP_FUNCTION) {
		char *returning = type_name(tp->returning);

		Vector *params = new_vector();
		for (int i = 0; i < tp->params->len; i++) {
			Type *p = tp->params->data[i];
			char *s = type_name(p);
			vec_push(params, s);
		}
		char *param_str = "";
		if (0 < tp->params->len) {
			param_str = string_join(params, ", ");
		}

		char *s = malloc(strlen("func() ") + strlen(param_str) +
				 strlen(returning) + 1);
		sprintf(s, "func(%s) %s", param_str, returning);
		return s;
	}
	if (tp->kind == TP_ARRAY) {
		char *src = type_name(tp->ptr_to);
		char *s =
		    malloc(strlen(src) + 30);  // sufficient for array_size
		sprintf(s, "[%d]%s", tp->array_size, src);
		return s;
	}
	if (tp->kind == TP_UNDETERMINED) return "undetermined";
	return "UNKNOWN (i.e. not implemented)";
}

int type_size(Type *tp) {
	if (tp->kind == TP_INT) return 4;
	if (tp->kind == TP_POINTER) return 8;
	if (tp->kind == TP_FUNCTION) return 8;
	if (tp->kind == TP_ARRAY) return type_size(tp->ptr_to) * tp->array_size;
	return -1000000007;
}

int type_size_refering(Type *tp) {
	if (tp->kind != TP_POINTER)
		error("type_size_refering(): must be pointer");
	return type_size(tp->ptr_to);
}
