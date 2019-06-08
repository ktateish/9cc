#!/bin/bash

try() {
	expected="$1"
	input="$2"

	mkdir -p /tmp/9cc-test
	sfile=$(mktemp /tmp/9cc-test/XXXXXXXX.s)
	trap "rm -f $sfile" RETURN
	./9cc "$input" > "$sfile"

	exefile=$(mktemp /tmp/9cc-test/XXXXXXXX)
	trap "rm -f $exefile $sfile" RETURN
	gcc -o "$exefile" "$sfile"
	"$exefile"
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

echo OK
