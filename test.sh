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

	./9cc "$input" > tmp.s
	gcc -o tmp  tmp.s foo.c
	./tmp
	actual="$?"

	if [ "$actual" = "$expected" ]; then
		echo "$input => $actual"
	else
		echo "$expected expected, but got $actual"
		exit 1
	fi
}

try 0 "0;"
try 42 "42;"
try 21 "5+20-4;"
try 41 " 12 + 34 - 5;"
try 47 "5+6*7;"
try 15 "5*(9-6);"
try 4 "(3+5)/2;"
try 2 "-3 + 5;"
try 3 "11 + -(3 + 5);"
try 4 "19 + -3 * +5;"

try 1 "1 + 2 == 3 - 0;"
try 0 "1 + 2 == 3 + 4;"

try 1 "2 * 3 != 4 / 5;"
try 0 "2 * 3 != 6;"

try 1 "2 < 3;"
try 0 "3 < 2;"

try 1 "3 <= 3;"
try 1 "3 <= 4;"
try 0 "5 <= 4;"

try 0 "2 > 3;"
try 1 "3 > 2;"

try 1 "3 >= 3;"
try 0 "3 >= 4;"
try 1 "5 >= 4;"

try 7 "1+2; 3+4;"

try 11 "a=5; b=6; a+b;"
try 36 "a=2; b=3; 3*5; a = a*b; a = a*a;3+5;a;"

try 3 "return 3;"
try 3 "return 3; return 5;"

try 3 "foo = 3; foo;"
try 6 "foo = 1; bar = 2 + 3; return foo + bar;"

try 3 "if (1) return 3; return 5;"
try 5 "if (0) return 3; return 5;"

try 1 "foo = 0; if (1+1) foo = 1; return foo;"
try 0 "foo = 0; if (1-1) foo = 1; return foo;"

try 8 "foo = 2; if (1) foo = foo*2; bar = foo+foo; return bar;"

try 7 "if (0) return 3; if (0) return 5; return 7;"
try 7 "if (0) return 3; if (0) return 5; return 7;"

try 3 "if (1) return 3; else return 5; return 7;"
try 5 "if (0) return 3; else return 5; return 7;"

try 0 "foo = 0; while (0) foo = foo+1; return foo;"
try 5 "foo = 0; while (foo < 5) foo = foo+1; return foo;"

try 5 "for (;;) return 5;"
try 4 "foo = 0; for (i = 0; i < 5; i = i+1) foo = i; return foo;"

try 5 "{ return 5; }"
try 5 "for (;;) { return 5; }"
try 5 "foo = 0; i = 0; while (i < 5) { foo = foo+1; i = i+1; } return foo;"

try 8 "bar = 3; return foo() + bar;"

echo OK
