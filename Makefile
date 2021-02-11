#
# Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
#

#
# Disable default implicit rules
#
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

export SHELL = /bin/bash

CC=clang
AR=ar
CFLAGS=-g -fstrict-aliasing -fstack-protector-all -pedantic -Wall -Wextra -Werror -Wfatal-errors --coverage
LLVM_COV=$(shell compgen -c | grep llvm-cov | sort | head -n 1)
ALL_SRC=$(wildcard *.c *.h)

tests.out: tests.a bitset.a buddy_alloc.a buddy_alloc_tree.a
	$(CC) -static $(CFLAGS) $^ -o tests.out
	rm -f *.gcda
	./tests.out
	$(LLVM_COV) gcov -b $(ALL_SRC) | paste -s -d ',' | sed -e 's/,,/,\n/' | cut -d ',' -f 1,2,3
	! grep  '#####:' *.gcov
	! grep -E '^branch\s*[0-9]? never executed$$' *.gcov

define DEP =
$$(shell $(CC) -MM -MG $(1))
	$(CC) $(CFLAGS) -c $(1) -o $$@
endef

$(foreach c_src,$(wildcard *.c),$(eval $(call DEP,$(c_src))))

define static_clang_tidy
	clang-tidy -checks='*,-llvm-header-guard,-llvm-include-order,-bugprone-assert-side-effect' -warnings-as-errors='*' $^ --
endef

%.static_clang_tidy:  %.c
	$(static_clang_tidy)

%.static_clang_tidy:  %.c %.h
	$(static_clang_tidy)

define static_ccpcheck
	cppcheck --error-exitcode=1 --quiet $^
endef

%.static_ccpcheck: %.c
	$(static_ccpcheck)

%.static_ccpcheck: %.c %.h
	$(static_ccpcheck)

%.static_checks: %.static_clang_tidy %.static_ccpcheck

%.a: %.o %.static_clang_tidy %.static_ccpcheck
	$(AR) rcs $@ $*.o

clean:
	rm -f *.a *.o *.gcda *.gcno *.gcov tests.out

# Mark clean as phony
.PHONY: clean

# Do not remove any intermediate files
.SECONDARY:
