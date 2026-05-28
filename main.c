#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

typedef int (__cdecl *archiveint_compress_file_fn)(const char *codec, const char *input, const char *output, int level);
typedef int (__cdecl *archiveint_decompress_file_fn)(const char *codec, const char *input, const char *output);

typedef struct {
    HMODULE module;
    archiveint_compress_file_fn compress_file;
    archiveint_decompress_file_fn decompress_file;
} archiveint_api;

typedef struct {
    const char *command;
    const char *codec;
    const char *default_ext;
} codec_map;

static const codec_map k_codecs[] = {
    {"gzip", "gzip", ".gz"},
    {"bzip2", "bzip2", ".bz2"},
    {"xz", "xz", ".xz"},
    {"lzma", "lzma", ".lzma"},
    {"lz4", "lz4", ".lz4"},
};

static const codec_map *find_codec(const char *command) {
    for (size_t i = 0; i < sizeof(k_codecs) / sizeof(k_codecs[0]); ++i) {
        if (strcmp(k_codecs[i].command, command) == 0) {
            return &k_codecs[i];
        }
    }
    return NULL;
}

static const char *basename_from_path(const char *path) {
    const char *a = strrchr(path, '/');
    const char *b = strrchr(path, '\\');
    const char *base = path;
    if (a && a + 1 > base) base = a + 1;
    if (b && b + 1 > base) base = b + 1;
    return base;
}

static void print_usage(const char *prog, const codec_map *c) {
    fprintf(stderr,
        "Usage: %s [OPTIONS] <input-file>\n"
        "       %s [OPTIONS] -o <output-file> <input-file>\n\n"
        "Options:\n"
        "  -d, --decompress       Decompress input\n"
        "  -k, --keep             Keep input file (accepted for compatibility)\n"
        "  -c, --stdout           Write to stdout (not supported)\n"
        "  -o, --output FILE      Output file path\n"
        "  -1 .. -9               Compression level\n"
        "  -h, --help             Show this help\n\n"
        "Command '%s' uses codec '%s'.\n",
        prog, prog, c->command, c->codec);
}

static char *derive_output_path(const codec_map *c, const char *input, bool decompress) {
    size_t in_len = strlen(input);
    size_t ext_len = strlen(c->default_ext);

    if (decompress) {
        if (in_len > ext_len && strcmp(input + in_len - ext_len, c->default_ext) == 0) {
            char *out = (char *)malloc(in_len - ext_len + 1);
            if (!out) return NULL;
            memcpy(out, input, in_len - ext_len);
            out[in_len - ext_len] = '\0';
            return out;
        }
        size_t out_len = in_len + 5;
        char *out = (char *)malloc(out_len + 1);
        if (!out) return NULL;
        snprintf(out, out_len + 1, "%s.out", input);
        return out;
    }

    char *out = (char *)malloc(in_len + ext_len + 1);
    if (!out) return NULL;
    memcpy(out, input, in_len);
    memcpy(out + in_len, c->default_ext, ext_len + 1);
    return out;
}

static bool load_archiveint(archiveint_api *api) {
    memset(api, 0, sizeof(*api));
    api->module = LoadLibraryA("archiveint.dll");
    if (!api->module) {
        fprintf(stderr, "archiveint.dll could not be loaded (GetLastError=%lu).\n", GetLastError());
        return false;
    }
    api->compress_file = (archiveint_compress_file_fn)GetProcAddress(api->module, "archiveint_compress_file");
    api->decompress_file = (archiveint_decompress_file_fn)GetProcAddress(api->module, "archiveint_decompress_file");
    if (!api->compress_file || !api->decompress_file) {
        fprintf(stderr,
                "Required exports not found in archiveint.dll.\n"
                "Expected: archiveint_compress_file, archiveint_decompress_file\n");
        FreeLibrary(api->module);
        memset(api, 0, sizeof(*api));
        return false;
    }
    return true;
}

int main(int argc, char **argv) {
    const char *prog = basename_from_path(argv[0]);
    const codec_map *codec = find_codec(prog);
    int arg_start = 1;

    if (!codec && argc > 1) {
        codec = find_codec(argv[1]);
        if (codec) {
            arg_start = 2;
        }
    }

    if (!codec) {
        fprintf(stderr,
                "Unknown command '%s'. Use one of: gzip, bzip2, xz, lzma, lz4\n"
                "You can invoke this program via command aliases (gzip.exe, etc.)\n"
                "or by using an explicit subcommand:\n"
                "  %s <codec> [options] <input-file>\n",
                (argc > 1 ? argv[1] : prog), prog);
        return 2;
    }

    bool decompress = false;
    int level = 6;
    const char *output_arg = NULL;

    static struct option long_opts[] = {
        {"decompress", no_argument, NULL, 'd'},
        {"keep", no_argument, NULL, 'k'},
        {"stdout", no_argument, NULL, 'c'},
        {"output", required_argument, NULL, 'o'},
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };

    int ch;
    optind = arg_start;
    while ((ch = getopt_long(argc, argv, "dko:ch123456789", long_opts, NULL)) != -1) {
        switch (ch) {
            case 'd': decompress = true; break;
            case 'o': output_arg = optarg; break;
            case 'k': break;
            case '1': case '2': case '3': case '4': case '5':
            case '6': case '7': case '8': case '9':
                level = ch - '0';
                break;
            case 'c':
                fprintf(stderr, "-c/--stdout is not supported in this implementation.\n");
                return 2;
            case 'h':
                print_usage(prog, codec);
                return 0;
            default:
                print_usage(prog, codec);
                return 2;
        }
    }

    if (optind >= argc) {
        print_usage(prog, codec);
        return 2;
    }
    const char *input = argv[optind];

    char *derived_output = NULL;
    const char *output = output_arg;
    if (!output) {
        derived_output = derive_output_path(codec, input, decompress);
        if (!derived_output) {
            fprintf(stderr, "Memory allocation failed: %s\n", strerror(errno));
            return 1;
        }
        output = derived_output;
    }

    archiveint_api api;
    if (!load_archiveint(&api)) {
        free(derived_output);
        return 1;
    }

    int rc = decompress
           ? api.decompress_file(codec->codec, input, output)
           : api.compress_file(codec->codec, input, output, level);

    if (rc != 0) {
        fprintf(stderr, "archiveint operation failed (code=%d).\n", rc);
    }

    FreeLibrary(api.module);
    free(derived_output);
    return rc == 0 ? 0 : 1;
}
