# Authors: Fatima Nadeem, Himanshi Lalwani
# OS Programming Assignment 3
# The makefile

CC = gcc

all: reader writer main

reader: reader.c
	$(CC) reader.c -o reader -lpthread

writer: writer.c
	$(CC) writer.c -o writer -lpthread

main: main.c
	$(CC) -o main main.c -lpthread

clean:
	rm -f reader writer main