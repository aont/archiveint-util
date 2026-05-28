# archiveint-util

Windows向けに `gzip` / `bzip2` / `xz` / `lzma` / `lz4` 互換のコマンド名で動作する薄いラッパーです。実処理は `archiveint.dll` の公開関数を呼び出します。

## 前提

- MSYS2 UCRT64 環境
- `archiveint.dll` が実行時に見つかること
- DLLが次の公開関数を持つこと
  - `int archiveint_compress_file(const char *codec, const char *input, const char *output, int level)`
  - `int archiveint_decompress_file(const char *codec, const char *input, const char *output)`

## ビルド

```bash
make
make install-links
```

`make install-links` により `gzip.exe` など5種類のコマンド名を同一実行ファイルから作成します。

## 使い方

```bash
gzip file.txt
bzip2 -9 file.txt
xz -d file.txt.xz
lz4 -o out.lz4 in.bin
```

主なオプション:

- `-d`, `--decompress`
- `-o`, `--output FILE`
- `-1`〜`-9`
- `-h`, `--help`

互換目的で `-k/--keep` は受理します（本実装では入力削除動作は行いません）。
