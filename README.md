# archiveint-util

`archiveint-util` is a small command-line wrapper around `archiveint.dll` for compressing and decompressing single files using raw archive filters.

## Supported codecs

- `gzip`
- `bzip2`
- `xz`
- `lzma`
- `lzip`
- `compress`
- `lz4`
- `lzop`
- `lrzip`
- `grzip`

## Build

```bash
make -f Makefile.mingw
```

Or with MSVC:

```cmd
nmake /f Makefile.msvc
```

Both makefiles build `archiveint-util.exe` without linking the C runtime by using a custom entry point and default-library suppression.

## Usage

```text
archiveint-util <gzip|bzip2|xz|lzma|lzip|compress|lz4|lzop|lrzip|grzip> [OPTION]... [FILE]
```

Common options:

- `-d, --decompress` – decompress mode
- `-c, --stdout` – write output to standard output
- `-f, --force` – overwrite output file
- `-1 .. -9` / `--fast` / `--best` – compression level
- `-i, --input FILE` – input file
- `-o, --output FILE` – output file

By default, output names are derived from codec extension (`.gz`, `.bz2`, `.xz`, `.lzma`, `.lz`, `.Z`, `.lz4`, `.lzo`, `.lrz`, `.grz`).

## Notes

- `archiveint.dll` must be available at runtime.
- The tool expects raw-stream behavior from the archive library APIs it loads dynamically.
