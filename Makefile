CFLAGS	:= -std=c99 -Wall -Wpedantic -D_GNU_SOURCE -g

.PHONY: all
all: bfjit

bfjit: bfjit.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

.PHONY: clean
clean:
	rm -f bfjit *.o
