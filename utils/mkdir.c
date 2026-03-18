#include <stdio.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "\033[38;5;201mUsage: %s <directory_name>\033[0m\n", argv[0]);
        return 1;
    }

    const char *dirname = argv[1];
    int status = mkdir(dirname, 0755);

    if (status != 0) {
        perror("mkdir");
        return 1;
    }

    return 0;
}
