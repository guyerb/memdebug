# -*- MakeFile -*-

subdirs = src test

all:
	for d in $(subdirs); do [ -d $$d ] && $(MAKE) -C $$d; done

check:
	for d in $(subdirs); do [ -d $$d ] && $(MAKE) -C $$d check; done

clean:
	for d in $(subdirs); do [ -d $$d ] && $(MAKE) -C $$d clean; done
	rm -f *~

