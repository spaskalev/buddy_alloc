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
CTIDY_CHECKS='*,-llvm-header-guard,-llvm-include-order,-bugprone-assert-side-effect,-readability-else-after-return'

LLVM_COV=$(shell compgen -c | grep llvm-cov | sort | head -n 1)
ALL_SRC=$(wildcard *.c *.h)
C_SRC=$(wildcard *.c)
STATIC=$(subst .c,.static,$(C_SRC))
OBJECTS=$(subst .c,.o,$(C_SRC))
GCOV_EXCLUDE=buddy_brk.c buddy_brk.h buddy_global.h
GCOV_SRC=$(filter-out $(GCOV_EXCLUDE), $(ALL_SRC))

tests.out: $(OBJECTS) $(STATIC)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@
	rm -f *.gcda
	./$@
	$(LLVM_COV) gcov -b $(GCOV_SRC) | paste -s -d ',' | sed -e 's/,,/,\n/' | cut -d ',' -f 1,2,3
	! grep  '#####:' *.gcov
	! grep -E '^branch\s*[0-9]? never executed$$' *.gcov

# The tr .. | xargs contraption bellow comes as a result of both
# gcc/cland respecting some maximum line width and putting extra
# dependencies on a new line using \ to indicate a line continuation.
# Make in turn messes this up via its shell handling.
define DEP =
$$(shell bash -c "$(CC) -MM -MG $(1) | tr -d '\\\\' | xargs ")
	$(CC) $(CFLAGS) -c $(1) -o $$@
endef

$(foreach src,$(wildcard *.c),$(eval $(call DEP,$(src))))

%.static_clang_tidy:  %.c %.h
	clang-tidy -checks=$(CTIDY_CHECKS) -warnings-as-errors='*' $^ --

%.static_ccpcheck: %.c %.h
	cppcheck --error-exitcode=1 --quiet $^

%.static: %.static_clang_tidy %.static_ccpcheck
	touch $@

clean:
	rm -f *.a *.o *.gcda *.gcno *.gcov *.static tests.out

# Mark clean as phony
.PHONY: clean

# Do not remove any intermediate files
.SECONDARY:

.PRECIOUS: tests.out
