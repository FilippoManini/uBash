CC = gcc
CFLAGS = -ggdb -Og -std=gnu11 -Wall -Werror -pedantic -fsanitize=address

uBash = uBash
uBash_OBJS = uBash.o 

EXECS = $(uBash)

all: $(EXECS)
.PHONY: clean tgz

$(uBash): $(uBash_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(uBash_OBJS): uBash.c
	$(CC) $(CFLAGS) -o $@ -c uBash.c

#Utilities
clean:
	rm -rf $(EXECS) *.o

tgz: clean
	cd ..; tar cvzf uBash.tgz uBash 
