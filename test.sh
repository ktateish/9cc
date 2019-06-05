#!/bin/bash

try() {
	expected="$1"
	input="$2"

	./9cc "$input" > tmp.s
	gcc -o tmp tmp.s
	./tmp
	actual="$?"

	if [ "$actual" = "$expected" ]; then
		echo "$input => $actual"
	else
		echo "$expected expected, but got $actual"
		exit 1
	fi
}

try 0 0
try 42 42
try 21 "5+20-4"
try 41 " 12 + 34 - 5 "
try 47 "5+6*7"
try 15 "5*(9-6)"
try 4 "(3+5)/2"
try 2 "-3 + 5"
try 3 "11 + -(3 + 5)"
try 4 "19 + -3 * +5"

try 1 "1 + 2 == 3 - 0"
try 0 "1 + 2 == 3 + 4"

try 1 "2 * 3 != 4 / 5"
try 0 "2 * 3 != 6"

try 1 "2 < 3"
try 0 "3 < 2"

try 1 "3 <= 3"
try 1 "3 <= 4"
try 0 "5 <= 4"

echo OK
