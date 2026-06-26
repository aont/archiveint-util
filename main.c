#include <windows.h>
#include "archiveint_dll.h"

#define BUF_SIZE 65536

static int streq(const char* a, const char* b){
  while(*a && *b && *a==*b){ ++a; ++b; }
  return *a==*b;
}

static unsigned long slen(const char* s){
  unsigned long n=0;
  while(s && s[n]) ++n;
  return n;
}

static int has_suffix(const char* s, const char* suffix){
  unsigned long n=slen(s), sn=slen(suffix), i;
  if(n<=sn) return 0;
  for(i=0;i<sn;++i){ if(s[n-sn+i]!=suffix[i]) return 0; }
  return 1;
}

static void* xalloc(unsigned long bytes){
  return HeapAlloc(GetProcessHeap(), 0, bytes);
}

static void write_all(HANDLE h, const char* s){
  DWORD written;
  WriteFile(h, s, slen(s), &written, NULL);
}

static void write_err(const char* s){
  write_all(GetStdHandle(STD_ERROR_HANDLE), s);
}

static void write_err3(const char* a, const char* b, const char* c){
  write_err(a); write_err(b); write_err(c);
}

static char** parse_command_line(int* argc_out){
  const char* p=GetCommandLineA();
  int argc=0, cap=8;
  char** argv=(char**)xalloc((unsigned long)cap*sizeof(char*));
  if(!argv) return NULL;
  while(*p){
    char* arg;
    unsigned long len=0, cap_arg=32;
    int in_quotes=0;
    while(*p==' ' || *p=='\t') ++p;
    if(!*p) break;
    if(argc>=cap){
      char** next;
      cap*=2;
      next=(char**)HeapReAlloc(GetProcessHeap(), 0, argv, (unsigned long)cap*sizeof(char*));
      if(!next) return NULL;
      argv=next;
    }
    arg=(char*)xalloc(cap_arg);
    if(!arg) return NULL;
    while(*p){
      char ch=*p;
      if(ch=='"'){
        in_quotes=!in_quotes;
        ++p;
        continue;
      }
      if(!in_quotes && (ch==' ' || ch=='\t')) break;
      if(ch=='\\'){
        unsigned long slash_count=0;
        while(p[slash_count]=='\\') ++slash_count;
        if(p[slash_count]=='"'){
          unsigned long pairs=slash_count/2, i;
          for(i=0;i<pairs;++i){
            if(len+1>=cap_arg){ cap_arg*=2; arg=(char*)HeapReAlloc(GetProcessHeap(),0,arg,cap_arg); if(!arg) return NULL; }
            arg[len++]='\\';
          }
          if(slash_count&1){
            if(len+1>=cap_arg){ cap_arg*=2; arg=(char*)HeapReAlloc(GetProcessHeap(),0,arg,cap_arg); if(!arg) return NULL; }
            arg[len++]='"';
            p += slash_count + 1;
            continue;
          }
          p += slash_count;
          continue;
        }
      }
      if(len+1>=cap_arg){ cap_arg*=2; arg=(char*)HeapReAlloc(GetProcessHeap(),0,arg,cap_arg); if(!arg) return NULL; }
      arg[len++]=ch;
      ++p;
    }
    arg[len]='\0';
    argv[argc++]=arg;
    while(*p==' ' || *p=='\t') ++p;
  }
  *argc_out=argc;
  return argv;
}

static int set_filter(struct archive* w, enum codec c){
  switch(c){
    case C_GZIP: return archive_write_add_filter_gzip(w);
    case C_BZIP2: return archive_write_add_filter_bzip2(w);
    case C_XZ: return archive_write_add_filter_xz(w);
    case C_LZMA: return archive_write_add_filter_lzma(w);
    case C_LZIP: return archive_write_add_filter_lzip(w);
    case C_COMPRESS: return archive_write_add_filter_compress(w);
    case C_LZ4: return archive_write_add_filter_lz4(w);
    case C_LZOP: return archive_write_add_filter_lzop(w);
    case C_LRZIP: return archive_write_add_filter_lrzip(w);
    case C_GRZIP: return archive_write_add_filter_grzip(w);
  }
  return -1;
}

static const char* codec_name(enum codec c){
  switch(c){
    case C_GZIP: return "gzip";
    case C_BZIP2: return "bzip2";
    case C_XZ: return "xz";
    case C_LZMA: return "lzma";
    case C_LZIP: return "lzip";
    case C_COMPRESS: return "compress";
    case C_LZ4: return "lz4";
    case C_LZOP: return "lzop";
    case C_LRZIP: return "lrzip";
    case C_GRZIP: return "grzip";
  }
  return "";
}

static int compress_file(enum codec c, const char* in, const char* out, int level){
  HANDLE fi=streq(in,"-")?GetStdHandle(STD_INPUT_HANDLE):CreateFileA(in,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  LARGE_INTEGER sz;
  struct archive* w;
  struct archive_entry* e;
  char buf[BUF_SIZE];
  DWORD n;
  if(fi==INVALID_HANDLE_VALUE){ write_err3("failed to open input: ",in,"\n"); return 1; }
  if(streq(in,"-")) sz.QuadPart=0;
  else if(!GetFileSizeEx(fi,&sz)){ write_err("failed to get input size\n"); CloseHandle(fi); return 1; }
  w=archive_write_new(); if(!w){ if(!streq(in,"-")) CloseHandle(fi); return 1; }
  if(set_filter(w,c)!=ARCHIVE_OK){ if(!streq(in,"-")) CloseHandle(fi); archive_write_free(w); return 1; }
  if(level>=0){
    char lv[2]; lv[0]=(char)('0'+level); lv[1]='\0';
    if(archive_write_set_filter_option(w,codec_name(c),"compression-level",lv)!=ARCHIVE_OK){
      write_err("failed to set compression level\n");
      if(!streq(in,"-")) CloseHandle(fi);
      archive_write_free(w);
      return 1;
    }
  }
  if(archive_write_set_format_raw(w)!=ARCHIVE_OK || archive_write_open_filename(w,out)!=ARCHIVE_OK){ if(!streq(in,"-")) CloseHandle(fi); archive_write_free(w); return 1; }
  e=archive_entry_new();
  archive_entry_set_pathname(e,in);
  archive_entry_set_filetype(e,0100000);
  archive_entry_set_perm(e,0644);
  archive_entry_set_size(e,sz.QuadPart);
  if(archive_write_header(w,e)!=ARCHIVE_OK){ archive_entry_free(e); if(!streq(in,"-")) CloseHandle(fi); archive_write_close(w); archive_write_free(w); return 1; }
  while(ReadFile(fi,buf,sizeof(buf),&n,NULL) && n>0){ if(archive_write_data(w,buf,n)<0){ archive_entry_free(e); if(!streq(in,"-")) CloseHandle(fi); archive_write_close(w); archive_write_free(w); return 1; } }
  archive_entry_free(e); if(!streq(in,"-")) CloseHandle(fi); archive_write_close(w); archive_write_free(w); return 0;
}

static int decompress_file(const char* in, const char* out){
  HANDLE fo=streq(out,"-")?GetStdHandle(STD_OUTPUT_HANDLE):CreateFileA(out,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
  struct archive* r;
  struct archive_entry* e=NULL;
  char buf[BUF_SIZE];
  long long n;
  if(fo==INVALID_HANDLE_VALUE){ write_err3("failed to open output: ",out,"\n"); return 1; }
  r=archive_read_new(); if(!r){ if(!streq(out,"-")) CloseHandle(fo); return 1; }
  if(archive_read_support_filter_all(r)!=ARCHIVE_OK || archive_read_support_format_raw(r)!=ARCHIVE_OK || archive_read_open_filename(r,in,BUF_SIZE)!=ARCHIVE_OK){ archive_read_free(r); if(!streq(out,"-")) CloseHandle(fo); return 1; }
  if(archive_read_next_header(r,&e)!=ARCHIVE_OK){ archive_read_free(r); if(!streq(out,"-")) CloseHandle(fo); return 1; }
  while((n=archive_read_data(r,buf,sizeof(buf)))>0){ DWORD written; if(!WriteFile(fo,buf,(DWORD)n,&written,NULL) || written!=(DWORD)n){ write_err("failed to write output\n"); archive_read_free(r); if(!streq(out,"-")) CloseHandle(fo); return 1; } }
  archive_read_free(r); if(!streq(out,"-")) CloseHandle(fo); return 0;
}

static void usage(void){
  write_err("usage: archiveint-util <gzip|bzip2|xz|lzma|lzip|compress|lz4|lzop|lrzip|grzip> [OPTION]... [FILE]\n"
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
    case C_LZIP: return ".lz";
    case C_COMPRESS: return ".Z";
    case C_LZ4: return ".lz4";
    case C_LZOP: return ".lzo";
    case C_LRZIP: return ".lrz";
    case C_GRZIP: return ".grz";
  }
  return "";
}

static int run_main(int argc, char** argv){
  enum codec c;
  int d=0,stdout_mode=0,force=0,level=-1,i; char* in=NULL; char* out=NULL;
  if(argc < 2){ usage(); return 1; }
  if(streq(argv[1],"gzip")) c=C_GZIP;
  else if(streq(argv[1],"bzip2")) c=C_BZIP2;
  else if(streq(argv[1],"xz")) c=C_XZ;
  else if(streq(argv[1],"lzma")) c=C_LZMA;
  else if(streq(argv[1],"lzip")) c=C_LZIP;
  else if(streq(argv[1],"compress")) c=C_COMPRESS;
  else if(streq(argv[1],"lz4")) c=C_LZ4;
  else if(streq(argv[1],"lzop")) c=C_LZOP;
  else if(streq(argv[1],"lrzip")) c=C_LRZIP;
  else if(streq(argv[1],"grzip")) c=C_GRZIP;
  else { usage(); return 1; }

  for(i=2;i<argc;++i){
    char* a=argv[i];
    if(streq(a,"-d") || streq(a,"--decompress")) d=1;
    else if(streq(a,"-c") || streq(a,"--stdout")) stdout_mode=1;
    else if(streq(a,"-k") || streq(a,"--keep")){}
    else if(streq(a,"-f") || streq(a,"--force")) force=1;
    else if(streq(a,"--fast")) level=1;
    else if(streq(a,"--best")) level=9;
    else if(a[0]=='-' && a[1]>='1' && a[1]<='9' && a[2]=='\0') level=a[1]-'0';
    else if((streq(a,"-i") || streq(a,"--input")) && i+1<argc) in=argv[++i];
    else if((streq(a,"-o") || streq(a,"--output")) && i+1<argc) out=argv[++i];
    else if(!in) in=a;
    else { usage(); return 1; }
  }
  if(!in){ usage(); return 1; }
  if(!out){
    const char* ext=codec_ext(c);
    unsigned long n=slen(in), exn=slen(ext);
    out=(char*)xalloc(n+exn+1);
    if(!out){ write_err("out of memory\n"); return 1; }
    if(d && has_suffix(in,ext)){
      unsigned long j;
      for(j=0;j<n-exn;++j) out[j]=in[j];
      out[n-exn]='\0';
    }else{
      unsigned long j;
      for(j=0;j<n;++j) out[j]=in[j];
      for(j=0;j<=exn;++j) out[n+j]=ext[j];
    }
  }
  if(stdout_mode) out="-";
  if(!stdout_mode && !force && GetFileAttributesA(out)!=INVALID_FILE_ATTRIBUTES){ write_err3(out," exists; use -f to overwrite","\n"); return 1; }
  return d ? decompress_file(in,out) : compress_file(c,in,out,level);
}

void mainCRTStartup(void){
  int argc=0;
  int rc;
  char** argv=parse_command_line(&argc);
  rc=argv ? run_main(argc,argv) : 1;
  ExitProcess((UINT)rc);
}
