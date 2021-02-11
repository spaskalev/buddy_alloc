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
C_SRC=$(wildcard *.c)
STATIC=$(subst .c,.static,$(C_SRC))
OBJECTS=$(subst .c,.o,$(C_SRC))

tests.out: $(OBJECTS) $(STATIC)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@
	rm -f *.gcda
	./$@
	$(LLVM_COV) gcov -b $(ALL_SRC) | paste -s -d ',' | sed -e 's/,,/,\n/' | cut -d ',' -f 1,2,3
	! grep  '#####:' *.gcov
	! grep -E '^branch\s*[0-9]? never executed$$' *.gcov

define DEP =
$$(shell $(CC) -MM -MG $(1))
	$(CC) $(CFLAGS) -c $(1) -o $$@
endef

$(foreach c_src,$(wildcard *.c),$(eval $(call DEP,$(c_src))))

%.static_clang_tidy:  %.c
	clang-tidy -checks='*,-llvm-header-guard,-llvm-include-order,-bugprone-assert-side-effect' -warnings-as-errors='*' $^ --

%.static_ccpcheck: %.c
	cppcheck --error-exitcode=1 --quiet $^

%.static: %.static_clang_tidy %.static_ccpcheck
	touch $@

clean:
	rm -f *.a *.o *.gcda *.gcno *.gcov tests.out

# Mark clean as phony
.PHONY: clean

# Do not remove any intermediate files
.SECONDARY:
