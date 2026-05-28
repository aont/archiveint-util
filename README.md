# archiveint-util

A lightweight Windows wrapper that provides `gzip` / `bzip2` / `xz` / `lzma` / `lz4` compatible command behavior.
Actual compression and decompression are delegated to exported functions in `archiveint.dll`.

## Requirements

- MSYS2 UCRT64 environment
- `archiveint.dll` must be discoverable at runtime
- The DLL must export these functions:
  - `int archiveint_compress_file(const char *codec, const char *input, const char *output, int level)`
  - `int archiveint_decompress_file(const char *codec, const char *input, const char *output)`

## Build

```bash
make
make install-links
```

`make install-links` creates five command aliases (`gzip.exe`, etc.) from the same executable.

## Usage

```bash
# Subcommand form
ai-codec.exe gzip file.txt
ai-codec.exe bzip2 -9 file.txt
ai-codec.exe xz -d file.txt.xz

# Alias form
gzip file.txt
bzip2 -9 file.txt
xz -d file.txt.xz
lz4 -o out.lz4 in.bin
```

Main options:

- `-d`, `--decompress`
- `-o`, `--output FILE`
- `-1` to `-9`
- `-h`, `--help`

For compatibility, `-k/--keep` is accepted, but this implementation does not remove the input file.
