#
# Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
#

# Disable default implicit rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

CC=clang
CFLAGS=-g -fstrict-aliasing -fstack-protector-all -pedantic -Wall -Wextra -Werror -Wfatal-errors --coverage
LLVM_COV=llvm-cov-7
CTIDY_CHECKS='*,-llvm-header-guard,-llvm-include-order,-bugprone-assert-side-effect,-readability-else-after-return'
MAKEFLAGS += --output-sync=target --jobs 4
TESTS_SRC=tests.c
LIB_SRC=buddy_alloc.h

test: tests.out
	rm -f *.gcda
	./tests.out
	$(LLVM_COV) gcov -b $(TESTS_SRC) | paste -s -d ',' | sed -e 's/,,/,\n/' | cut -d ',' -f 1,2,3
	! grep  '#####:' *.gcov
	! grep -E '^branch\s*[0-9]? never executed$$' *.gcov

tests.out: $(TESTS_SRC) $(LIB_SRC) test-clang-tidy test-cppcheck
	$(CC) $(CFLAGS) $(TESTS_SRC) -o $@

test-clang-tidy: $(TESTS_SRC)
	clang-tidy -checks=$(CTIDY_CHECKS) -warnings-as-errors='*' $^ --

test-cppcheck: $(TESTS_SRC)
	cppcheck --error-exitcode=1 --quiet $^

clean:
	rm -f *.gcda *.gcno *.gcov tests.out

.PHONY: test clean test-clang-tidy test-cppcheck

.PRECIOUS: tests.out
