#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#define BUF_SIZE 65536
#include "archiveint_dll.h"

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
  FILE* fi=(strcmp(in,"-")==0)?stdin:fopen(in,"rb"); if(!fi){perror("fopen input"); return 1;}
  if(fseek(fi,0,SEEK_END)!=0){perror("fseek"); if(fi!=stdin) fclose(fi); return 1;}
  long long sz=ftell(fi); rewind(fi);
  struct archive* w=archive_write_new(); if(!w){fclose(fi); return 1;}
  if(set_filter(w,c)!=ARCHIVE_OK){if(fi!=stdin) fclose(fi); archive_write_free(w); return 1;}
  if(level>=0){
    char lv[4];
    snprintf(lv,sizeof(lv),"%d",level);
    if(archive_write_set_filter_option(w,codec_name(c),"compression-level",lv)!=ARCHIVE_OK){
      fprintf(stderr,"failed to set compression level %d for %s\n",level,codec_name(c));
      if(fi!=stdin) fclose(fi);
      archive_write_free(w);
      return 1;
    }
  }
  if(archive_write_set_format_raw(w)!=ARCHIVE_OK || archive_write_open_filename(w,out)!=ARCHIVE_OK){if(fi!=stdin) fclose(fi); archive_write_free(w); return 1;}
  struct archive_entry* e=archive_entry_new();
  archive_entry_set_pathname(e,in);
  archive_entry_set_filetype(e,0100000);
  archive_entry_set_perm(e,0644);
  archive_entry_set_size(e,sz);
  if(archive_write_header(w,e)!=ARCHIVE_OK){archive_entry_free(e); if(fi!=stdin) fclose(fi); archive_write_close(w); archive_write_free(w); return 1;}
  char buf[BUF_SIZE]; size_t n;
  while((n=fread(buf,1,sizeof(buf),fi))>0){ if(archive_write_data(w,buf,n)<0){archive_entry_free(e); if(fi!=stdin) fclose(fi); archive_write_close(w); archive_write_free(w); return 1;} }
  archive_entry_free(e); if(fi!=stdin) fclose(fi); archive_write_close(w); archive_write_free(w); return 0;
}

static int decompress_file(const char* in, const char* out){
  FILE* fo=(strcmp(out,"-")==0)?stdout:fopen(out,"wb"); if(!fo){perror("fopen output"); return 1;}
  struct archive* r=archive_read_new(); if(!r){fclose(fo); return 1;}
  if(archive_read_support_filter_all(r)!=ARCHIVE_OK || archive_read_support_format_raw(r)!=ARCHIVE_OK || archive_read_open_filename(r,in,BUF_SIZE)!=ARCHIVE_OK){archive_read_free(r); fclose(fo); return 1;}
  struct archive_entry* e=NULL;
  if(archive_read_next_header(r,&e)!=ARCHIVE_OK){archive_read_free(r); if(fo!=stdout) fclose(fo); return 1;}
  char buf[BUF_SIZE]; long long n;
  while((n=archive_read_data(r,buf,sizeof(buf)))>0){ if(fwrite(buf,1,(size_t)n,fo)!=(size_t)n){perror("fwrite"); archive_read_free(r); if(fo!=stdout) fclose(fo); return 1;} }
  archive_read_free(r); if(fo!=stdout) fclose(fo); return 0;
}

static void usage(void){
  fprintf(stderr,
    "usage: archiveint-util <gzip|bzip2|xz|lzma|lzip|compress|lz4|lzop|lrzip|grzip> [OPTION]... [FILE]\n"
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

int main(int argc, char** argv){
  if(argc < 2){ usage(); return 1; }
  enum codec c;
  if(strcmp(argv[1],"gzip")==0) c=C_GZIP;
  else if(strcmp(argv[1],"bzip2")==0) c=C_BZIP2;
  else if(strcmp(argv[1],"xz")==0) c=C_XZ;
  else if(strcmp(argv[1],"lzma")==0) c=C_LZMA;
  else if(strcmp(argv[1],"lzip")==0) c=C_LZIP;
  else if(strcmp(argv[1],"compress")==0) c=C_COMPRESS;
  else if(strcmp(argv[1],"lz4")==0) c=C_LZ4;
  else if(strcmp(argv[1],"lzop")==0) c=C_LZOP;
  else if(strcmp(argv[1],"lrzip")==0) c=C_LRZIP;
  else if(strcmp(argv[1],"grzip")==0) c=C_GRZIP;
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

  int rc = d ? decompress_file(in,out) : compress_file(c,in,out,level);
  return rc;
}
