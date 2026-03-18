#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "\033[38;5;201mUsage: %s FILE...\033[0m\n", argv[0]);
        return 1;
    }

    int exit_code = 0;

    for (int i = 1; i < argc; ++i) {
        if (unlink(argv[i]) != 0) {
            fprintf(stderr, "\033[38;5;196mrm: cannot remove '%s': %s\033[0m\n", argv[i], strerror(errno));
            exit_code = 1;
        }
    }

    return exit_code;
}
