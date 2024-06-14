cmake_minimum_required(VERSION 3.10)
project(buddy_alloc)
set(C_STANDARD C99)

# Compile C tests
project(buddy_tests)
set(C_STANDARD C99)
set(SOURCE_FILES tests.c)
add_executable(buddy_tests ${SOURCE_FILES})

# Compile CPP translation unit
project(buddy_cpp_translation_unit)
set(CXX_STANDARD C++11)
set(SOURCE_FILES testcxx.cpp)
add_executable(buddy_cpp_translation_unit ${SOURCE_FILES})

# Compile benchmark
project(buddy_bench)
set(C_STANDARD C99)
set(SOURCE_FILES bench.c)
add_executable(buddy_bench ${SOURCE_FILES})