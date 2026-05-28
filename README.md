# archiveint-util

`archiveint-util` is a small command-line wrapper around `archiveint.dll` for compressing and decompressing single files using raw archive filters.

## Supported codecs

- `gzip`
- `bzip2`
- `xz`
- `lzma`
- `lz4`

## Build

```bash
make
```

This produces `archiveint-util.exe`.

## Usage

```text
archiveint-util <gzip|bzip2|xz|lzma|lz4> [OPTION]... [FILE]
```

Common options:

- `-d, --decompress` – decompress mode
- `-c, --stdout` – write output to standard output
- `-f, --force` – overwrite output file
- `-1 .. -9` / `--fast` / `--best` – compression level
- `-i, --input FILE` – input file
- `-o, --output FILE` – output file

By default, output names are derived from codec extension (`.gz`, `.bz2`, `.xz`, `.lzma`, `.lz4`).

## Notes

- `archiveint.dll` must be available at runtime.
- The tool expects raw-stream behavior from the archive library APIs it loads dynamically.
