@rem
@rem Microsoft Public License (Ms-PL) - Copyright (c) 2020 Sean Moss
@rem This file is subject to the terms and conditions of the Microsoft Public License, the text of which can be found in
@rem the 'LICENSE' file at the root of this repository, or online at <https://opensource.org/licenses/MS-PL>.
@rem

@rem This is the syntax parser/lexer generator script for Windows.

@echo off

java -jar ../tools/antlr-4.9-complete.jar ^
    -no-listener                          ^
    -visitor                              ^
    -o ../generated                       ^
    -package grammar                      ^
    -Xexact-output-dir                    ^
    -Dlanguage=Cpp                        ^
    VSLLexer.g4

java -jar ../tools/antlr-4.9-complete.jar ^
    -no-listener                          ^
    -visitor                              ^
    -o ../generated                       ^
    -package grammar                      ^
    -Xexact-output-dir                    ^
    -Dlanguage=Cpp                        ^
    VSL.g4
