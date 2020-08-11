CFLAGS	:= -std=c99 -Wall -Wpedantic -D_GNU_SOURCE -g

.PHONY: all
all: bfjit

bfjit: bfjit.o gen_amd64.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

.PHONY: clean
clean:
	rm -f bfjit *.o

test: bfjit
	./bfjit test/mandelbrot.b > helloworld.asm
	nasm -f elf64 helloworld.asm
	ld helloworld.o -o helloworld
