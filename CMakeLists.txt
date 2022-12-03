cmake_minimum_required(VERSION 3.10)
project(buddy_alloc)
set(C_STANDARD C99)

# Add test.cpp file of project root directory as source file
set(SOURCE_FILES tests.c)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(buddy_alloc ${SOURCE_FILES})