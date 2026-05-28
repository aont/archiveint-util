#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#define BUF_SIZE 65536
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

static void init_api(struct api* a){
  memset(a,0,sizeof(*a));
  a->write_new = archive_write_new;
  a->write_free = archive_write_free;
  a->write_open_filename = archive_write_open_filename;
  a->write_close = archive_write_close;
  a->write_header = archive_write_header;
  a->write_data = archive_write_data;
  a->add_gzip = archive_write_add_filter_gzip;
  a->add_bzip2 = archive_write_add_filter_bzip2;
  a->add_xz = archive_write_add_filter_xz;
  a->add_lzma = archive_write_add_filter_lzma;
  a->add_lz4 = archive_write_add_filter_lz4;
  a->set_raw = archive_write_set_format_raw;
  a->set_filter_option = archive_write_set_filter_option;
  a->entry_new = archive_entry_new;
  a->entry_free = archive_entry_free;
  a->entry_set_pathname = archive_entry_set_pathname;
  a->entry_set_filetype = archive_entry_set_filetype;
  a->entry_set_perm = archive_entry_set_perm;
  a->entry_set_size = archive_entry_set_size;
  a->read_new = archive_read_new;
  a->read_free = archive_read_free;
  a->read_open_filename = archive_read_open_filename;
  a->read_next_header = archive_read_next_header;
  a->read_data = archive_read_data;
  a->support_format_raw = archive_read_support_format_raw;
  a->support_filter_all = archive_read_support_filter_all;
}

static int set_filter(struct api* a, struct archive* w, enum codec c){
  switch(c){
    case C_GZIP: return a->add_gzip(w);
    case C_BZIP2: return a->add_bzip2(w);
    case C_XZ: return a->add_xz(w);
    case C_LZMA: return a->add_lzma(w);
    case C_LZ4: return a->add_lz4(w);
  }
  return -1;
}

static const char* codec_name(enum codec c){
  switch(c){
    case C_GZIP: return "gzip";
    case C_BZIP2: return "bzip2";
    case C_XZ: return "xz";
    case C_LZMA: return "lzma";
    case C_LZ4: return "lz4";
  }
  return "";
}

static int compress_file(struct api* a, enum codec c, const char* in, const char* out, int level){
  FILE* fi=(strcmp(in,"-")==0)?stdin:fopen(in,"rb"); if(!fi){perror("fopen input"); return 1;}
  if(fseek(fi,0,SEEK_END)!=0){perror("fseek"); if(fi!=stdin) fclose(fi); return 1;}
  long long sz=ftell(fi); rewind(fi);
  struct archive* w=a->write_new(); if(!w){fclose(fi); return 1;}
  if(set_filter(a,w,c)!=ARCHIVE_OK){if(fi!=stdin) fclose(fi); a->write_free(w); return 1;}
  if(level>=0){
    char lv[4];
    snprintf(lv,sizeof(lv),"%d",level);
    if(a->set_filter_option(w,codec_name(c),"compression-level",lv)!=ARCHIVE_OK){
      fprintf(stderr,"failed to set compression level %d for %s\n",level,codec_name(c));
      if(fi!=stdin) fclose(fi); a->write_free(w); return 1;
    }
  }
  if(a->set_raw(w)!=ARCHIVE_OK || a->write_open_filename(w,out)!=ARCHIVE_OK){if(fi!=stdin) fclose(fi); a->write_free(w); return 1;}
  struct archive_entry* e=a->entry_new();
  a->entry_set_pathname(e,in);
  a->entry_set_filetype(e,0100000);
  a->entry_set_perm(e,0644);
  a->entry_set_size(e,sz);
  if(a->write_header(w,e)!=ARCHIVE_OK){a->entry_free(e); if(fi!=stdin) fclose(fi); a->write_close(w); a->write_free(w); return 1;}
  char buf[BUF_SIZE]; size_t n;
  while((n=fread(buf,1,sizeof(buf),fi))>0){ if(a->write_data(w,buf,n)<0){a->entry_free(e); if(fi!=stdin) fclose(fi); a->write_close(w); a->write_free(w); return 1;} }
  a->entry_free(e); if(fi!=stdin) fclose(fi); a->write_close(w); a->write_free(w); return 0;
}

static int decompress_file(struct api* a, const char* in, const char* out){
  FILE* fo=(strcmp(out,"-")==0)?stdout:fopen(out,"wb"); if(!fo){perror("fopen output"); return 1;}
  struct archive* r=a->read_new(); if(!r){fclose(fo); return 1;}
  if(a->support_filter_all(r)!=ARCHIVE_OK || a->support_format_raw(r)!=ARCHIVE_OK || a->read_open_filename(r,in,BUF_SIZE)!=ARCHIVE_OK){a->read_free(r); fclose(fo); return 1;}
  struct archive_entry* e=NULL;
  if(a->read_next_header(r,&e)!=ARCHIVE_OK){a->read_free(r); if(fo!=stdout) fclose(fo); return 1;}
  char buf[BUF_SIZE]; long long n;
  while((n=a->read_data(r,buf,sizeof(buf)))>0){ if(fwrite(buf,1,(size_t)n,fo)!=(size_t)n){perror("fwrite"); a->read_free(r); if(fo!=stdout) fclose(fo); return 1;} }
  a->read_free(r); if(fo!=stdout) fclose(fo); return 0;
}

static void usage(void){
  fprintf(stderr,
    "usage: archiveint-util <gzip|bzip2|xz|lzma|lz4> [OPTION]... [FILE]\n"
    "  -d, --decompress        decompress\n"
    "  -c, --stdout            write to standard output\n"
    "  -k, --keep              keep input files (default behavior)\n"
    "  -f, --force             overwrite output files\n"
    "  -1 .. -9                compression level (fast..best)\n"
    "      --fast              same as -1\n"
    "      --best              same as -9\n"
    "  -i, --input FILE        input file (or positional FILE)\n"
    "  -o, --output FILE       output file\n");
}

static const char* codec_ext(enum codec c){
  switch(c){
    case C_GZIP: return ".gz";
    case C_BZIP2: return ".bz2";
    case C_XZ: return ".xz";
    case C_LZMA: return ".lzma";
    case C_LZ4: return ".lz4";
  }
  return "";
}

int main(int argc, char** argv){
  if(argc < 2){ usage(); return 1; }
  enum codec c;
  if(strcmp(argv[1],"gzip")==0) c=C_GZIP;
  else if(strcmp(argv[1],"bzip2")==0) c=C_BZIP2;
  else if(strcmp(argv[1],"xz")==0) c=C_XZ;
  else if(strcmp(argv[1],"lzma")==0) c=C_LZMA;
  else if(strcmp(argv[1],"lz4")==0) c=C_LZ4;
  else { usage(); return 1; }

  int d=0,opt,stdout_mode=0,force=0,level=-1; char* in=NULL; char* out=NULL;
  static struct option opts[]={ {"decompress",0,0,'d'},{"stdout",0,0,'c'},{"keep",0,0,'k'},{"force",0,0,'f'},{"fast",0,0,1000},{"best",0,0,1001},{"input",1,0,'i'},{"output",1,0,'o'},{0,0,0,0} };
  optind=2;
  while((opt=getopt_long(argc,argv,"dckf123456789i:o:",opts,NULL))!=-1){
    if(opt=='d') d=1;
    else if(opt=='c') stdout_mode=1;
    else if(opt=='k'){}
    else if(opt=='f') force=1;
    else if(opt>='1'&&opt<='9') level=opt-'0';
    else if(opt==1000) level=1;
    else if(opt==1001) level=9;
    else if(opt=='i') in=optarg;
    else if(opt=='o') out=optarg;
    else {usage(); return 1;}
  }
  if(!in && optind<argc) in=argv[optind];
  if(!in){ usage(); return 1; }
  if(!out){
    const char* ext=codec_ext(c);
    size_t n=strlen(in), exn=strlen(ext);
    out=(char*)malloc(n+exn+1);
    if(!out){ perror("malloc"); return 1; }
    if(d && n>exn && strcmp(in+n-exn,ext)==0){
      memcpy(out,in,n-exn);
      out[n-exn]='\0';
    }else{
      memcpy(out,in,n);
      memcpy(out+n,ext,exn+1);
    }
  }
  if(stdout_mode){ out="-"; }

  if(!stdout_mode && !force){
    FILE* t=fopen(out,"rb");
    if(t){ fclose(t); fprintf(stderr,"%s exists; use -f to overwrite\n",out); return 1; }
  }

  struct api api;
  init_api(&api);
  int rc = d ? decompress_file(&api,in,out) : compress_file(&api,c,in,out,level);
  return rc;
}
