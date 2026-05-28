CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c11
LDFLAGS ?=

TARGET = ai-codec.exe

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

install-links: $(TARGET)
	cp -f $(TARGET) gzip.exe
	cp -f $(TARGET) bzip2.exe
	cp -f $(TARGET) xz.exe
	cp -f $(TARGET) lzma.exe
	cp -f $(TARGET) lz4.exe

clean:
	rm -f $(TARGET) gzip.exe bzip2.exe xz.exe lzma.exe lz4.exe
