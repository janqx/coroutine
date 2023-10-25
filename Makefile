all: example

example: example.c coroutine.c
	gcc -std=c99 -Wall -g -o $@ $^
	./example

clean:
	rm ./example
