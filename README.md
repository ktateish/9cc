# 9cc - C compiler based on the Rui Ueyama's Compiler Book

[9cc][1] is a small, simple, easy to understand C compiler developed by Rui Ueyama.
This repositry is my implementation of 9cc based on [his book '低レイヤを知りたい人のためのCコンパイラ作成入門'][2].

## TODO

* [x] Calculator-like language
    * [x] Compile a single integer
    * [x] Addition and Subtraction
    * [x] Tokenizer
    * [x] Multiplication and Division
    * [x] Unary `+` and `-`
    * [x] Comparison operators
    * [x] Support arbitrary length input
* [x] Separated compilation
* [ ] Variables and functions
    * [x] Single character local variables
    * [x] Support `return` statement
    * [x] Multi-character local variables
    * [x] Control structures
        * if
        * while
        * for
    * [x] Block
    * [x] Function call
    * [ ] Function definition
* [ ] Pointer and string literal

[1]: https://github.com/rui314/9cc
[2]: https://www.sigbus.info/compilerbook
