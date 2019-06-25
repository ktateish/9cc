#!/bin/bash

mkdir -p /tmp/9cc-test
d=$(mktemp -d /tmp/9cc-test/XXXXXXXX)
trap "rm -rf $d; rmdir /tmp/9cc-test 2>/dev/null" EXIT

cp 9cc $d/9cc
cd $d

try() {
	expected="$1"
	input="$2"

	cat <<EOM > external.c
int ex1() { return 5; }

int ex2(int a) { return a * 2; }

int ex3(int a1, int a2, int a3, int a4, int a5, int a6) {
	return a1 + a2 + a3 + a4 + a5 + a6;
}
EOM

	./9cc "$input" > tmp.s
	gcc -o tmp  tmp.s external.c
	./tmp
	actual="$?"

	if [ "$actual" = "$expected" ]; then
		echo "$input => $actual"
	else
		echo "$expected expected, but got $actual"
		exit 1
	fi
}

try 0 "int main() { return 0; }"
try 42 "int main() { return 42; }"
try 21 "int main() { return 5+20-4; }"
try 41 "int main() { return  12 + 34 - 5; }"
try 47 "int main() { return 5+6*7; }"
try 15 "int main() { return 5*(9-6); }"
try 4 "int main() { return (3+5)/2; }"
try 2 "int main() { return -3 + 5; }"
try 3 "int main() { return 11 + -(3 + 5); }"
try 4 "int main() { return 19 + -3 * +5; }"

try 1 "int main() { return 1 + 2 == 3 - 0; }"
try 0 "int main() { return 1 + 2 == 3 + 4; }"

try 1 "int main() { return 2 * 3 != 4 / 5; }"
try 0 "int main() { return 2 * 3 != 6; }"

try 1 "int main() { return 2 < 3; }"
try 0 "int main() { return 3 < 2; }"

try 1 "int main() { return 3 <= 3; }"
try 1 "int main() { return 3 <= 4; }"
try 0 "int main() { return 5 <= 4; }"

try 0 "int main() { return 2 > 3; }"
try 1 "int main() { return 3 > 2; }"

try 1 "int main() { return 3 >= 3; }"
try 0 "int main() { return 3 >= 4; }"
try 1 "int main() { return 5 >= 4; }"

try 7 "int main() { 1+2; return 3+4; }"

try 11 "int main() { int a; int b; a=5; b=6; return a+b; }"
try 36 "int main() { int a; int b; a=2; b=3; 3*5; a = a*b; a = a*a;3+5; return a; }"
try 2 "int main() { int a; int b; a = b = 2; return a; }"

try 3 "int main() { return 3; }"
try 3 "int main() { return 3; return 5; }"

try 3 "int main() { int foo; foo = 3; return foo; }"
try 6 "int main() { int foo; int bar; foo = 1; bar = 2 + 3; return foo + bar; }"

try 3 "int main() { if (1) return 3; return 5; }"
try 5 "int main() { if (0) return 3; return 5; }"

try 1 "int main() { int foo; foo = 0; if (1+1) foo = 1; return foo; }"
try 0 "int main() { int foo; foo = 0; if (1-1) foo = 1; return foo; }"

try 8 "int main() { int foo; int bar; foo = 2; if (1) foo = foo*2; bar = foo+foo; return bar; }"

try 7 "int main() { if (0) return 3; if (0) return 5; return 7; }"
try 7 "int main() { if (0) return 3; if (0) return 5; return 7; }"

try 3 "int main() { if (1) return 3; else return 5; return 7; }"
try 5 "int main() { if (0) return 3; else return 5; return 7; }"

try 0 "int main() { int foo; foo = 0; while (0) foo = foo+1; return foo; }"
try 5 "int main() { int foo; foo = 0; while (foo < 5) foo = foo+1; return foo; }"

try 5 "int main() { for (;;) return 5; }"
try 4 "int main() { int foo; int i; foo = 0; for (i = 0; i < 5; i = i+1) foo = i; return foo; }"

try 5 "int main() { { return 5; } }"
try 5 "int main() { for (;;) { return 5; } }"
try 5 "int main() { int foo; int i; foo = 0; i = 0; while (i < 5) { foo = foo+1; i = i+1; } return foo; }"

try 8 "int ex1(); int main() { int bar; bar = 3; return ex1() + bar; }"

try 9 "int ex2(int a); int main() { int a; a = 2; return ex2(a) + 5; }"
try 28 "int ex3(int a1, int a2, int a3, int a4, int a5, int a6); int main() { return ex3(1, 2, 3, 4, 5, 6) + 7; }"
try 28 "int ex3(int a1, int a2, int a3, int a4, int a5, int a6); int main() { int a; a = ex3(1, 2, 3, 4, 5, 6) + 7; return a; }"

try 4 "int f() { return 2; } int main() { return 2 * f(); }"

try 25 "int f(int a) { return a*a; } int main() { return f(3) + f(4); }"
try 28 "int f(int a1, int a2, int a3, int a4, int a5, int a6) { return a1 + a2 + a3 + a4 + a5 + a6; } int main() { return f(1, 2, 3, 4, 5, 6) + 7; }"
try 55 "int fib(int n) { if (n == 0) return 0; if (n == 1) return 1; return fib(n-1) + fib(n-2); }  int main() { return fib(10); }"

try 7 "int main() {int a; int *b; a = 7; b = &a; return *b;}"
try 8 "int main() {int a; a = 7; int *b; b = &a; *b = *b + 1; return a; }"
try 8 "int main() {int a; a = 7; int *b; b = &a; int **c; c = &b; **c = **c + 1; return a; }"
try 8 "int f(int *a) { *a = *a + 1; return 0;} int main() {int b; b = 7; f(&b); return b; }"
try 9 "int f(int *a) { *a = *a + 1; return 0;} int g(int **b) { **b = **b+1; f(*b); return 0; } int main() {int c; c = 7; int *d; d = &c; g(&d); return c; }"

try 3 "int main() { int a; int b; a = 1; { int a; a= 2; b = a; } return a+b; }"
try 10 "int main() { int a; int b; int c; a = 1; { int a; a = 2; b = a; { int b; int c; b = 100; c = 200; a = b+c; }} { int a; int b; a = 3; b = 4; c = a+b; } return a+b+c; }"


echo OK
