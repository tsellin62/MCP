CC = gcc
CFLAGS = -Wall -Wextra -g

all: part1 iobound cpubound

part1: part1.o string_parser.o
	$(CC) $(CFLAGS) -o part1 part1.o string_parser.o

part1.o: part1.c string_parser.h
	$(CC) $(CFLAGS) -c part1.c

string_parser.o: string_parser.c string_parser.h
	$(CC) $(CFLAGS) -c string_parser.c

iobound: iobound.c
	$(CC) $(CFLAGS) -o iobound iobound.c

cpubound: cpubound.c
	$(CC) $(CFLAGS) -o cpubound cpubound.c

clean:
	rm -f *.o part1 iobound cpubound
