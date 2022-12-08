cmake_minimum_required(VERSION 3.10)
project(buddy_alloc)

# Compile C tests
project(buddy_tests)
set(C_STANDARD C99)

# Add tests.c
set(SOURCE_FILES tests.c)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(buddy_tests ${SOURCE_FILES})

# Compile CPP translation unit
project(buddy_cpp_translation_unit)

# Add test_cpp.cpp
set(SOURCE_FILES testcxx.cpp)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(buddy_cpp_translation_unit ${SOURCE_FILES})