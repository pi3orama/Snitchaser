all: loader.so test test-2 test-3

CFLAGS += -g -std=gnu99 -Wall -nostartfiles -nodefaultlibs -nostdlib -fno-builtin -fPIC # -fvisibility=hidden -fPIC

loader.so: loader.o libprintf.os
	ld -shared $^ -o $@

loader.o: loader.c
vsprintf.o: vsprintf.c
string_32.o: string_32.c
__ctype.o: __ctype.c
libprintf.os: vsprintf.o string_32.o __ctype.o
	ld -r $^ -o $@




test: test.o
	gcc -Wl,-dynamic-linker,$(shell pwd)/loader.so $< -o $@

test-2: test.o
	gcc -Wl,-dynamic-linker,/tmp/ld-linux.so.2 $< -o $@

test-3: test.o
	gcc $< -o $@

test.o: test.c
	gcc -c $< -o $@

.PHONY: clean

clean:
	rm -f *.o *.so *.os test

