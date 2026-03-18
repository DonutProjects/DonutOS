#include <stdio.h>

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
            perror("fwrite");
            fclose(in);
            fclose(out);
            return 1;
        }
    }

    fclose(in);
    fclose(out);

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
        fprintf(stderr, "\033[38;5;196mmv: failed to move '%s' to '%s'\033[0m\n", src, dst);
        return 1;
    }

    return 0;
}
