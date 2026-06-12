#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

static int print_file(FILE *fp, const char *filename) {
    char buffer[4096];
    size_t bytes;

    while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        if (fwrite(buffer, 1, bytes, stdout) != bytes) {
            fprintf(stderr, "\033[38;5;196mcat: error writing to stdout\033[0m\n");
            return 1;
        }
    }

    if (ferror(fp)) {
        fprintf(stderr, "\033[38;5;196mcat: error reading from %s\033[0m\n", filename);
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        return print_file(stdin, "stdin");
    } else {
        int exit_code = 0;
        for (int i = 1; i < argc; ++i) {
            FILE *fp = fopen(argv[i], "r");
            if (!fp) {
                fprintf(stderr, "\033[38;5;196mcat: cannot open %s: %s\033[0m\n", argv[i], strerror(errno));
                exit_code = 1;
                continue;
            }
            if (print_file(fp, argv[i]) != 0) {
                exit_code = 1;
            }
            fclose(fp);
        }
        return exit_code;
    }
}
