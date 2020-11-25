# -*- MakeFile -*-

uname := $(shell uname -s)

ifeq ($(uname), Linux)
	target = libinject.so
endif
ifeq ($(uname), Darwin)
	target = libinject.dylib
endif

%.o: %.c
	gcc -c  $<  -fPIC  -o $@

$(target): read.o
	gcc  $^  -shared  -o $@

clean:
	rm -f *.o  $(target)  *~

check: $(target)

