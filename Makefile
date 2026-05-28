CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c11

all: archiveint-util.exe

archiveint-util.exe: main.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f archiveint-util.exe
