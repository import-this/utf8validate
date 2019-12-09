CC=gcc

all: main.c
	$(CC) -Wall -Wextra -Wshadow -Wconversion -pedantic -o utf8validate main.c

clean:
	rm -rf utf8validate
