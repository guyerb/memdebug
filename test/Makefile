# -*- MakeFile -*-
#libdir := $(CURDIR)/../src
#gcc -g   $^  -o $@ -L $(libdir)  -ldmalloc

target = checks

uname := $(shell uname -s)
ifeq ($(uname), Linux)
	preload = LD_PRELOAD=../src/libdmalloc.so
endif
ifeq ($(uname), Darwin)
	preload = DYLD_INSERT_LIBRARIES=../src/libdmalloc.dylib DYLD_FORCE_FLAT_NAMESPACE=1
endif


%.o: %.c
	gcc -g -std=c99 -Wall -Wextra  -c $<  -o $@

$(target): checks.o
	gcc -g   $^  -o $@

check: $(target)
	@echo Run 1 with standard malloc calls
	@echo ----------------------------
	./$(target)
	@echo ----------------------------
	@echo
	@echo Run 2 with injected malloc calls
	@echo ----------------------------
	 $(preload) ./$(target)
	@echo ----------------------------
	@echo

clean distclean:
	rm -f  *.o  $(target)  *~

