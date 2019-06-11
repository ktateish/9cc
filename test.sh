#!/bin/bash

mkdir -p /tmp/9cc-test
d=$(mktemp -d /tmp/9cc-test/XXXXXXXX)
trap "rm -rf $d; rmdir /tmp/9cc-test 2>/dev/null" EXIT

cp 9cc $d/9cc
cd $d

try() {
	expected="$1"
	input="$2"

	cat <<EOM > foo.c
int foo() { return 5; }
EOM

	cat <<EOM > bar.c
int bar(int a) { return a * 2; }
EOM

	cat <<EOM > buz.c
int buz(int a1, int a2, int a3, int a4, int a5, int a6) {
	return a1 + a2 + a3 + a4 + a5 + a6;
}
EOM

	./9cc "$input" > tmp.s
	gcc -o tmp  tmp.s foo.c bar.c buz.c
	./tmp
	actual="$?"

	if [ "$actual" = "$expected" ]; then
		echo "$input => $actual"
	else
		echo "$expected expected, but got $actual"
		exit 1
	fi
}

try 0 "main() { return 0; }"
try 42 "main() { return 42; }"
try 21 "main() { return 5+20-4; }"
try 41 "main() { return  12 + 34 - 5; }"
try 47 "main() { return 5+6*7; }"
try 15 "main() { return 5*(9-6); }"
try 4 "main() { return (3+5)/2; }"
try 2 "main() { return -3 + 5; }"
try 3 "main() { return 11 + -(3 + 5); }"
try 4 "main() { return 19 + -3 * +5; }"

try 1 "main() { return 1 + 2 == 3 - 0; }"
try 0 "main() { return 1 + 2 == 3 + 4; }"

try 1 "main() { return 2 * 3 != 4 / 5; }"
try 0 "main() { return 2 * 3 != 6; }"

try 1 "main() { return 2 < 3; }"
try 0 "main() { return 3 < 2; }"

try 1 "main() { return 3 <= 3; }"
try 1 "main() { return 3 <= 4; }"
try 0 "main() { return 5 <= 4; }"

try 0 "main() { return 2 > 3; }"
try 1 "main() { return 3 > 2; }"

try 1 "main() { return 3 >= 3; }"
try 0 "main() { return 3 >= 4; }"
try 1 "main() { return 5 >= 4; }"

try 7 "main() { 1+2; return 3+4; }"

try 11 "main() { a=5; b=6; return a+b; }"
try 36 "main() { a=2; b=3; 3*5; a = a*b; a = a*a;3+5; return a; }"

try 3 "main() { return 3; }"
try 3 "main() { return 3; return 5; }"

try 3 "main() { foo = 3; return foo; }"
try 6 "main() { foo = 1; bar = 2 + 3; return foo + bar; }"

try 3 "main() { if (1) return 3; return 5; }"
try 5 "main() { if (0) return 3; return 5; }"

try 1 "main() { foo = 0; if (1+1) foo = 1; return foo; }"
try 0 "main() { foo = 0; if (1-1) foo = 1; return foo; }"

try 8 "main() { foo = 2; if (1) foo = foo*2; bar = foo+foo; return bar; }"

try 7 "main() { if (0) return 3; if (0) return 5; return 7; }"
try 7 "main() { if (0) return 3; if (0) return 5; return 7; }"

try 3 "main() { if (1) return 3; else return 5; return 7; }"
try 5 "main() { if (0) return 3; else return 5; return 7; }"

try 0 "main() { foo = 0; while (0) foo = foo+1; return foo; }"
try 5 "main() { foo = 0; while (foo < 5) foo = foo+1; return foo; }"

try 5 "main() { for (;;) return 5; }"
try 4 "main() { foo = 0; for (i = 0; i < 5; i = i+1) foo = i; return foo; }"

try 5 "main() { { return 5; } }"
try 5 "main() { for (;;) { return 5; } }"
try 5 "main() { foo = 0; i = 0; while (i < 5) { foo = foo+1; i = i+1; } return foo; }"

try 8 "main() { bar = 3; return foo() + bar; }"

try 9 "main() { a = 2; return bar(a) + 5; }"
try 28 "main() { return buz(1, 2, 3, 4, 5, 6) + 7; }"
try 28 "main() { a = buz(1, 2, 3, 4, 5, 6) + 7; return a; }"

try 4 "f() { return 2; } main() { return 2 * f(); }"

try 25 "f(a) { return a*a; } main() { return f(3) + f(4); }"
try 28 "f(a1, a2, a3, a4, a5, a6) { return a1 + a2 + a3 + a4 + a5 + a6; } main() { return f(1, 2, 3, 4, 5, 6) + 7; }"
try 55 "fib(n) { if (n == 0) return 0; if (n == 1) return 1; return fib(n-1) + fib(n-2); }  main() { return fib(10); }"


echo OK
