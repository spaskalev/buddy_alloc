#
# Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
#

# Disable default implicit rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

LLVM_VERSION?=11
CC?=clang-$(LLVM_VERSION)
CXX?=clang++-$(LLVM_VERSION)
CFLAGS?=-std=c99 --coverage -pg -no-pie -fno-builtin -g -O0 -Og -fstrict-aliasing -fstack-protector-all -fsanitize=undefined -fsanitize=bounds -pedantic -Wall -Wextra -Werror -Wfatal-errors -Wformat=2 -Wformat-security -Wconversion -Wcast-qual -Wnull-dereference -Wstack-protector -Warray-bounds -Warray-bounds-pointer-arithmetic -Wassign-enum -Wbad-function-cast -Wconditional-uninitialized -Wfloat-equal -Wformat-type-confusion -Widiomatic-parentheses -Wimplicit-fallthrough -Wloop-analysis -Wpointer-arith -Wshift-sign-overflow -Wshorten-64-to-32 -Wswitch-enum -Wtautological-constant-in-range-compare -Wunreachable-code-aggressive -Wthread-safety -Wthread-safety-beta -Wcomma -Wdeclaration-after-statement -D_FORTIFY_SOURCE=3
CXXFLAGS?=-std=c++11 --coverage -pg -no-pie -fno-builtin -g -O0 -Og -fstrict-aliasing -fstack-protector-all -fsanitize=undefined -fsanitize=bounds -pedantic -Wall -Wextra -Wformat=2 -Wformat-security -Wconversion -Wcast-qual -Wnull-dereference -Wstack-protector -Warray-bounds -Warray-bounds-pointer-arithmetic -Wassign-enum -Wbad-function-cast -Wconditional-uninitialized -Wfloat-equal -Wformat-type-confusion -Widiomatic-parentheses -Wimplicit-fallthrough -Wloop-analysis -Wpointer-arith -Wshift-sign-overflow -Wshorten-64-to-32 -Wswitch-enum -Wtautological-constant-in-range-compare -Wunreachable-code-aggressive -Wthread-safety -Wthread-safety-beta -Wcomma -Wdeclaration-after-statement -D_FORTIFY_SOURCE=3
LLVM_COV?=llvm-cov-$(LLVM_VERSION)
CPPCHECK?=cppcheck

TESTS_SRC=tests.c
TESTCXX_SRC=testcxx.cpp
LIB_SRC=buddy_alloc.h
BENCH_SRC=bench.c
BENCH_CFLAGS?=-O2

test: tests.out
	rm -f *.gcda
	./tests.out
	$(LLVM_COV) gcov -b $(TESTS_SRC) | paste -s -d ',' | sed -e 's/,,/,\n/' | cut -d ',' -f 1,2,3
	! grep  '#####:' $(LIB_SRC).gcov
	! grep -E '^branch\s*[0-9]? never executed$$' $(LIB_SRC).gcov

tests.out: $(TESTS_SRC) $(LIB_SRC) test-cppcheck check-recursion
	$(CC) $(CFLAGS) $(TESTS_SRC) -o $@

test-cppcheck: $(TESTS_SRC)
	$(CPPCHECK) --error-exitcode=1 --quiet $^

test-cpp-translation-unit: $(TESTCXX_SRC)
	$(CXX) $(CXXFLAGS) $(TESTCXX_SRC) -o $@

test-multiplatform: $(TESTS_SRC)
	# 64-bit
	powerpc64-linux-gnu-gcc -static $(TESTS_SRC) && ./a.out
	aarch64-linux-gnu-gcc -static $(TESTS_SRC) && ./a.out
	# 32-bit
	i686-linux-gnu-gcc -static -g tests.c && ./a.out
	powerpc-linux-gnu-gcc -static $(TESTS_SRC) && ./a.out

bench: $(BENCH_SRC) $(LIB_SRC)
	$(CC) $(BENCH_CFLAGS) $(BENCH_SRC) -o $@
	./$@

check-recursion: $(LIB_SRC)
	[ $$( cflow --no-main $(LIB_SRC) | grep -c 'recursive:' ) -eq "0" ]

clean:
	rm -f a.out *.gcda *.gcno *.gcov tests.out bench

.PHONY: test clean test-cppcheck

.PRECIOUS: tests.out testcxx.out
