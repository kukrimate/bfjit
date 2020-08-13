# C compiler flags
CFLAGS	:= -std=gnu99 -Wall -D_GNU_SOURCE -g

# Objects
OBJ	:= src/bfjit.o src/il.o src/jit_amd64.o

.PHONY: all
all: bfjit

bfjit: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

.PHONY: clean
clean:
	rm -f bfjit $(OBJ)
