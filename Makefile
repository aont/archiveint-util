CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c11
LDFLAGS ?=
LDLIBS ?= c:/Windows/System32/archiveint.dll

all: archiveint-util.exe

archiveint-util.exe: main.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f archiveint-util.exe
