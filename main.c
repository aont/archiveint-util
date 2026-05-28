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

typedef struct archive* (__cdecl *fn_archive_write_new)(void);
typedef int (__cdecl *fn_archive_write_free)(struct archive*);
typedef int (__cdecl *fn_archive_write_open_filename)(struct archive*, const char*);
typedef int (__cdecl *fn_archive_write_close)(struct archive*);
typedef int (__cdecl *fn_archive_write_header)(struct archive*, struct archive_entry*);
typedef long long (__cdecl *fn_archive_write_data)(struct archive*, const void*, size_t);
typedef int (__cdecl *fn_archive_write_add_filter_gzip)(struct archive*);
typedef int (__cdecl *fn_archive_write_add_filter_bzip2)(struct archive*);
typedef int (__cdecl *fn_archive_write_add_filter_xz)(struct archive*);
typedef int (__cdecl *fn_archive_write_add_filter_lzma)(struct archive*);
typedef int (__cdecl *fn_archive_write_add_filter_lz4)(struct archive*);
typedef int (__cdecl *fn_archive_write_set_format_raw)(struct archive*);
typedef int (__cdecl *fn_archive_write_set_filter_option)(struct archive*, const char*, const char*, const char*);

typedef struct archive_entry* (__cdecl *fn_archive_entry_new)(void);
typedef void (__cdecl *fn_archive_entry_free)(struct archive_entry*);
typedef void (__cdecl *fn_archive_entry_set_pathname)(struct archive_entry*, const char*);
typedef void (__cdecl *fn_archive_entry_set_filetype)(struct archive_entry*, unsigned int);
typedef void (__cdecl *fn_archive_entry_set_perm)(struct archive_entry*, unsigned int);
typedef void (__cdecl *fn_archive_entry_set_size)(struct archive_entry*, long long);

typedef struct archive* (__cdecl *fn_archive_read_new)(void);
typedef int (__cdecl *fn_archive_read_free)(struct archive*);
typedef int (__cdecl *fn_archive_read_open_filename)(struct archive*, const char*, size_t);
typedef int (__cdecl *fn_archive_read_next_header)(struct archive*, struct archive_entry**);
typedef long long (__cdecl *fn_archive_read_data)(struct archive*, void*, size_t);
typedef int (__cdecl *fn_archive_read_support_format_raw)(struct archive*);
typedef int (__cdecl *fn_archive_read_support_filter_all)(struct archive*);

enum codec { C_GZIP, C_BZIP2, C_XZ, C_LZMA, C_LZ4 };

struct api {
  HMODULE dll;
  fn_archive_write_new write_new;
  fn_archive_write_free write_free;
  fn_archive_write_open_filename write_open_filename;
  fn_archive_write_close write_close;
  fn_archive_write_header write_header;
  fn_archive_write_data write_data;
  fn_archive_write_add_filter_gzip add_gzip;
  fn_archive_write_add_filter_bzip2 add_bzip2;
  fn_archive_write_add_filter_xz add_xz;
  fn_archive_write_add_filter_lzma add_lzma;
  fn_archive_write_add_filter_lz4 add_lz4;
  fn_archive_write_set_format_raw set_raw;
  fn_archive_write_set_filter_option set_filter_option;

  fn_archive_entry_new entry_new;
  fn_archive_entry_free entry_free;
  fn_archive_entry_set_pathname entry_set_pathname;
  fn_archive_entry_set_filetype entry_set_filetype;
  fn_archive_entry_set_perm entry_set_perm;
  fn_archive_entry_set_size entry_set_size;

  fn_archive_read_new read_new;
  fn_archive_read_free read_free;
  fn_archive_read_open_filename read_open_filename;
  fn_archive_read_next_header read_next_header;
  fn_archive_read_data read_data;
  fn_archive_read_support_format_raw support_format_raw;
  fn_archive_read_support_filter_all support_filter_all;
};

static FARPROC sym(HMODULE h, const char* n){ FARPROC p=GetProcAddress(h,n); if(p) return p; return NULL; }
#define LOAD(API, NAME) do{ api.NAME=(void*)sym(api.dll,#NAME); if(!api.NAME){fprintf(stderr,"missing symbol %s\n",#NAME); return 1;}}while(0)

static int load_api(struct api* a){
  memset(a,0,sizeof(*a));
  a->dll = LoadLibraryA("archiveint.dll");
  if(!a->dll){ fprintf(stderr,"could not load archiveint.dll\n"); return 1; }
#define L(name, field) do { a->field=(void*)sym(a->dll,name); if(!a->field){fprintf(stderr,"missing symbol %s\n",name); return 1;} } while(0)
  L("archive_write_new", write_new);
  L("archive_write_free", write_free);
  L("archive_write_open_filename", write_open_filename);
  L("archive_write_close", write_close);
  L("archive_write_header", write_header);
  L("archive_write_data", write_data);
  L("archive_write_add_filter_gzip", add_gzip);
  L("archive_write_add_filter_bzip2", add_bzip2);
  L("archive_write_add_filter_xz", add_xz);
  L("archive_write_add_filter_lzma", add_lzma);
  L("archive_write_add_filter_lz4", add_lz4);
  L("archive_write_set_format_raw", set_raw);
  L("archive_write_set_filter_option", set_filter_option);
  L("archive_entry_new", entry_new);
  L("archive_entry_free", entry_free);
  L("archive_entry_set_pathname", entry_set_pathname);
  L("archive_entry_set_filetype", entry_set_filetype);
  L("archive_entry_set_perm", entry_set_perm);
  L("archive_entry_set_size", entry_set_size);
  L("archive_read_new", read_new);
  L("archive_read_free", read_free);
  L("archive_read_open_filename", read_open_filename);
  L("archive_read_next_header", read_next_header);
  L("archive_read_data", read_data);
  L("archive_read_support_format_raw", support_format_raw);
  L("archive_read_support_filter_all", support_filter_all);
#undef L
  return 0;
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
  if(load_api(&api)!=0) return 1;
  int rc = d ? decompress_file(&api,in,out) : compress_file(&api,c,in,out,level);
  if(api.dll) FreeLibrary(api.dll);
  return rc;
}
