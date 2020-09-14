cmake_minimum_required(VERSION 3.4.1)

testBuilder_add_source(textEditDemo textEdit.cpp)
testBuilder_add_library(textEditDemo ncurses)
testBuilder_build(textEditDemo EXECUTABLES)
