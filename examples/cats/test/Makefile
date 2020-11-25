# -*- MakeFile -*-

target = cats

%.o: %.c
	gcc -Wall  -Werror  -c $<  -o $@

$(target): cats.o
	gcc $^  -o $@

check: $(target)
	@echo Run 1 with standard read call
	@echo ----------------------------
	./$(target)
	@echo ----------------------------
	@echo
	@sleep 1
	@echo Run 2 with injected read call
	@echo ----------------------------
	DYLD_INSERT_LIBRARIES=../src/libinject.dylib DYLD_FORCE_FLAT_NAMESPACE=1 ./$(target)
	@echo ----------------------------
	@echo

clean:
	rm -f  *.o  $(target)  *~

