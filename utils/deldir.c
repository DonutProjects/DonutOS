#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

int remove_recursive(const char *path) {
    struct stat st;

    if (lstat(path, &st) != 0) {
        fprintf(stderr, "\033[38;5;196mrmdir: cannot access '%s': %s\033[0m\n", path, strerror(errno));
        return -1;
    }

    if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(path);
        if (!dir) {
            fprintf(stderr, "\033[38;5;196mrmdir: cannot open directory '%s': %s\033[0m\n", path, strerror(errno));
            return -1;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char fullpath[4096];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

            if (remove_recursive(fullpath) != 0) {
                closedir(dir);
                return -1;
            }
        }

        closedir(dir);

        if (rmdir(path) != 0) {
            fprintf(stderr, "\033[38;5;196mrmdir: failed to remove directory '%s': %s\033[0m\n", path, strerror(errno));
            return -1;
        }

    } else {
        if (unlink(path) != 0) {
            fprintf(stderr, "\033[38;5;196mrmdir: failed to remove file '%s': %s\033[0m\n", path, strerror(errno));
            return -1;
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "\033[38;5;201mUsage: %s DIRECTORY...\033[0m\n", argv[0]);
        return 1;
    }

    int exit_code = 0;
    for (int i = 1; i < argc; ++i) {
        if (remove_recursive(argv[i]) != 0) {
            exit_code = 1;
        }
    }

    return exit_code;
}
