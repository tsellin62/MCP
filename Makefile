CC = gcc
CFLAGS = -Wall -Wextra -g

part1: part1.o string_parser.o
	$(CC) $(CFLAGS) -o part1 part1.o string_parser.o

part1.o: part1.c string_parser.h
	$(CC) $(CFLAGS) -c part1.c

string_parser.o: string_parser.c string_parser.h
	$(CC) $(CFLAGS) -c string_parser.c

clean:
	rm -f *.o part1
