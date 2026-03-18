#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

int list_dir(const char *path) {
    struct dirent *entry;
    DIR *dir = opendir(path);

    if (dir == NULL) {
        switch (errno) {
            case ENOENT:
                fprintf(stderr, "\033[38;5;196mError: No such file or directory: %s\033[0m\n", path);
                break;
            case EACCES:
                fprintf(stderr, "\033[38;5;196mError: Permission denied: %s\033[0m\n", path);
                break;
            default:
                fprintf(stderr, "\033[38;5;196mError: Cannot open directory '%s': %s\033[0m\n", path, strerror(errno));
        }
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            printf("%s\n", entry->d_name);
    }

    closedir(dir);
    return 0;
}

int main(int argc, char *argv[]) {
    const char *target = ".";

    if (argc > 2) {
        fprintf(stderr, "\033[38;5;201mUsage: %s [directory]\033[0m\n", argv[0]);
        return 1;
    }

    if (argc == 2)
        target = argv[1];

    return list_dir(target);
}
