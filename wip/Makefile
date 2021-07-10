STRESS_NG=~/dev/stress-ng/./stress-ng
#STRESS_NG=stress-ng

buddy_shared.so: buddy_brk.c buddy_brk.h buddy_global.h ../buddy_alloc.h
	gcc -g -O0 -Og -fpic -shared -o buddy_shared.so buddy_brk.c

test: buddy_shared.so
	env LD_PRELOAD=./buddy_shared.so $(STRESS_NG) --malloc 1 --malloc-ops 500 2>&1

trace: buddy_shared.so
	ltrace -S -l buddy_shared.so env LD_PRELOAD=./buddy_shared.so $(STRESS_NG) --malloc 1 --malloc-ops 1 2>&1

perf: buddy_shared.so
	rm -f rm perf.data out.perf-folded perf-kernel.svg
	sudo perf record -F 99 -g -- env LD_PRELOAD=./buddy_shared.so $(STRESS_NG) --malloc 1 --malloc-ops 100000 2>&1
	sudo chown $(shell whoami) perf.data
	perf script | ~/dev/FlameGraph/stackcollapse-perf.pl > out.perf-folded
	~/dev/FlameGraph/flamegraph.pl out.perf-folded > perf-kernel.svg
	x-www-browser file://$(shell pwd)/perf-kernel.svg

debug: buddy_shared.so
	gdb -x gdbinit.cfg --args $(STRESS_NG) --malloc 1 --malloc-ops 1 2>&1

clean:
	rm -f buddy_shared.so

.PHONY: test trace perf debug clean
