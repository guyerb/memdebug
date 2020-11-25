# -*- MakeFile -*-

objects := dmalloc.o dmalloc_stat.o dmalloc_api.o dmalloc_common.o
depends := $(objects:.o=.d)

uname := $(shell uname -s)
ifeq ($(uname), Linux)
	os = LINUX
	target = libdmalloc.so
	objects += dmalloc_wrappers_linux.o
endif
ifeq ($(uname), Darwin)
	os = DARWIN
	target = libdmalloc.dylib
endif

%.o: %.c
	gcc -g -std=c99 -Wall -Wextra -D $(os) -c  $<  -fPIC  -o $@
	@gcc -MM $*.c > $*.d

$(target): $(objects)
	gcc -g  $^  -shared  -o $@

check: $(target)

clean:
	rm -f *.o *.d *~

distclean: clean
	rm -f *.dylib *.so
	rm -rf *.dSYM

-include $(depends)

