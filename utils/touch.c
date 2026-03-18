#include <stdio.h>
#include <utime.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int touch_file(const char *filename) {
    int fd = open(filename, O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
        fprintf(stderr, "\033[38;5;196mtouch: cannot touch '%s': %s\033[0m\n", filename, strerror(errno));
        return 1;
    }
    close(fd);

    if (utime(filename, NULL) < 0) {
        fprintf(stderr, "\033[38;5;196mtouch: cannot update times for '%s': %s\033[0m\n", filename, strerror(errno));
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "\033[38;5;201mUsage: %s file...\033[0m\n", argv[0]);
        return 1;
    }

    int status = 0;
    for (int i = 1; i < argc; i++) {
        status |= touch_file(argv[i]);
    }

    return status;
}
