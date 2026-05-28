#ifndef ARCHIVEINT_DLL_H
#define ARCHIVEINT_DLL_H

#include <stddef.h>

#define ARCHIVE_OK 0
#define ARCHIVE_EOF 1

struct archive;
struct archive_entry;

extern struct archive* __cdecl archive_write_new(void);
extern int __cdecl archive_write_free(struct archive*);
extern int __cdecl archive_write_open_filename(struct archive*, const char*);
extern int __cdecl archive_write_close(struct archive*);
extern int __cdecl archive_write_header(struct archive*, struct archive_entry*);
extern long long __cdecl archive_write_data(struct archive*, const void*, size_t);
extern int __cdecl archive_write_add_filter_gzip(struct archive*);
extern int __cdecl archive_write_add_filter_bzip2(struct archive*);
extern int __cdecl archive_write_add_filter_xz(struct archive*);
extern int __cdecl archive_write_add_filter_lzma(struct archive*);
extern int __cdecl archive_write_add_filter_lz4(struct archive*);
extern int __cdecl archive_write_set_format_raw(struct archive*);
extern int __cdecl archive_write_set_filter_option(struct archive*, const char*, const char*, const char*);

extern struct archive_entry* __cdecl archive_entry_new(void);
extern void __cdecl archive_entry_free(struct archive_entry*);
extern void __cdecl archive_entry_set_pathname(struct archive_entry*, const char*);
extern void __cdecl archive_entry_set_filetype(struct archive_entry*, unsigned int);
extern void __cdecl archive_entry_set_perm(struct archive_entry*, unsigned int);
extern void __cdecl archive_entry_set_size(struct archive_entry*, long long);

extern struct archive* __cdecl archive_read_new(void);
extern int __cdecl archive_read_free(struct archive*);
extern int __cdecl archive_read_open_filename(struct archive*, const char*, size_t);
extern int __cdecl archive_read_next_header(struct archive*, struct archive_entry**);
extern long long __cdecl archive_read_data(struct archive*, void*, size_t);
extern int __cdecl archive_read_support_format_raw(struct archive*);
extern int __cdecl archive_read_support_filter_all(struct archive*);

enum codec { C_GZIP, C_BZIP2, C_XZ, C_LZMA, C_LZ4 };

struct api {
  struct archive* (__cdecl *write_new)(void);
  int (__cdecl *write_free)(struct archive*);
  int (__cdecl *write_open_filename)(struct archive*, const char*);
  int (__cdecl *write_close)(struct archive*);
  int (__cdecl *write_header)(struct archive*, struct archive_entry*);
  long long (__cdecl *write_data)(struct archive*, const void*, size_t);
  int (__cdecl *add_gzip)(struct archive*);
  int (__cdecl *add_bzip2)(struct archive*);
  int (__cdecl *add_xz)(struct archive*);
  int (__cdecl *add_lzma)(struct archive*);
  int (__cdecl *add_lz4)(struct archive*);
  int (__cdecl *set_raw)(struct archive*);
  int (__cdecl *set_filter_option)(struct archive*, const char*, const char*, const char*);

  struct archive_entry* (__cdecl *entry_new)(void);
  void (__cdecl *entry_free)(struct archive_entry*);
  void (__cdecl *entry_set_pathname)(struct archive_entry*, const char*);
  void (__cdecl *entry_set_filetype)(struct archive_entry*, unsigned int);
  void (__cdecl *entry_set_perm)(struct archive_entry*, unsigned int);
  void (__cdecl *entry_set_size)(struct archive_entry*, long long);

  struct archive* (__cdecl *read_new)(void);
  int (__cdecl *read_free)(struct archive*);
  int (__cdecl *read_open_filename)(struct archive*, const char*, size_t);
  int (__cdecl *read_next_header)(struct archive*, struct archive_entry**);
  long long (__cdecl *read_data)(struct archive*, void*, size_t);
  int (__cdecl *support_format_raw)(struct archive*);
  int (__cdecl *support_filter_all)(struct archive*);
};

#endif
