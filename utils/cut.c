#include <errno.h>
#include <stdio.h>
#include <string.h>

int move(const char *src, const char *dst) {
    if (rename(src, dst) == 0) {
        return 0;
    }

    FILE *in = fopen(src, "rb");
    if (!in) {
        perror("fopen(src)");
        return 1;
    }

    FILE *out = fopen(dst, "wb");
    if (!out) {
        perror("fopen(dst)");
        fclose(in);
        return 1;
    }

    char buf[8192];
    size_t bytes;
    while ((bytes = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, bytes, out) != bytes) {
            fprintf(stderr, "\033[38;5;196mcut: cannot write '%s': %s\033[0m\n", dst, strerror(errno));
            fclose(in);
            fclose(out);
            return 1;
        }
    }

    if (ferror(in)) {
        fprintf(stderr, "\033[38;5;196mcut: cannot read '%s': %s\033[0m\n", src, strerror(errno));
        fclose(in);
        fclose(out);
        return 1;
    }

    if (fflush(out) != 0) {
        fprintf(stderr, "\033[38;5;196mcut: cannot flush '%s': %s\033[0m\n", dst, strerror(errno));
        fclose(in);
        fclose(out);
        return 1;
    }

    if (fclose(out) != 0) {
        fprintf(stderr, "\033[38;5;196mcut: cannot close '%s': %s\033[0m\n", dst, strerror(errno));
        fclose(in);
        return 1;
    }

    if (fclose(in) != 0) {
        fprintf(stderr, "\033[38;5;196mcut: cannot close '%s': %s\033[0m\n", src, strerror(errno));
        return 1;
    }

    if (remove(src) != 0) {
        perror("remove");
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "\033[38;5;201mUsage: %s SOURCE DEST\033[0m\n", argv[0]);
        return 1;
    }

    const char *src = argv[1];
    const char *dst = argv[2];

    if (move(src, dst) != 0) {
        fprintf(stderr, "\033[38;5;196mcut: failed to move '%s' to '%s'\033[0m\n", src, dst);
        return 1;
    }

    return 0;
}
