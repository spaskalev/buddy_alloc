#
# Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
#

# Disable default implicit rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

CC=clang-11
CFLAGS=-std=c11 -g -fstrict-aliasing -fstack-protector-all -pedantic -Wall -Wextra -Werror -Wfatal-errors --coverage
LLVM_COV=llvm-cov-11
CTIDY=clang-tidy-11
CTIDY_CHECKS='bugprone-*,performance-*,readability-*,-readability-magic-numbers,-clang-analyzer-security.*'
CTIDY_EXTRA='-std=c11'
TESTS_SRC=tests.c
LIB_SRC=buddy_alloc.h

test: tests.out
	rm -f *.gcda
	./tests.out
	$(LLVM_COV) gcov -b $(TESTS_SRC) | paste -s -d ',' | sed -e 's/,,/,\n/' | cut -d ',' -f 1,2,3
	! grep  '#####:' *.gcov
	! grep -E '^branch\s*[0-9]? never executed$$' *.gcov

tests.out: $(TESTS_SRC) $(LIB_SRC) test-clang-tidy test-cppcheck check-recursion
	$(CC) $(CFLAGS) $(TESTS_SRC) -o $@

test-clang-tidy: $(TESTS_SRC)
	$(CTIDY) -checks=$(CTIDY_CHECKS) -warnings-as-errors='*' --extra-arg=$(CTIDY_EXTRA) $^ --

test-cppcheck: $(TESTS_SRC)
	cppcheck --error-exitcode=1 --quiet $^

check-recursion: $(LIB_SRC)
	[ $$( cflow --no-main $(LIB_SRC) | grep -c 'recursive:' ) -eq "0" ]

clean:
	rm -f *.gcda *.gcno *.gcov tests.out

.PHONY: test clean test-clang-tidy test-cppcheck

.PRECIOUS: tests.out
