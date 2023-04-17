# Authors: Fatima Nadeem, Himanshi Lalwani
# OS Programming Assignment 3
# The makefile

CC = gcc

all: reader writer main

reader: reader.c
	$(CC) reader.c -o reader

writer: writer.c
	$(CC) writer.c -o writer

main: main.c
	$(CC) -o main main.c

clean:
	rm -f reader writer main