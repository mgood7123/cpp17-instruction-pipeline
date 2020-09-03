cmake_minimum_required(VERSION 3.4.1)

testBuilder_add_source(pipeline pipeline_v3.cpp)
testBuilder_add_library(pipeline pthread)
testBuilder_add_library(pipeline easyloggingpp)
testBuilder_build(pipeline EXECUTABLES)
