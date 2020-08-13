# C compiler flags
CFLAGS	:= -Ikeystone/build/lib/include -Ilibkm \
	-std=c99 -Wall -Wpedantic -D_GNU_SOURCE -g

# Libraries to link
LDLIBS	:= -Lkeystone/build/lib/lib -lkeystone

# Objects
OBJ	:= src/bfjit.o src/il.o src/gen_amd64.o

.PHONY: all
all: bfjit

bfjit: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

.PHONY: clean
clean:
	rm -f bfjit $(OBJ)
