#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

static char *join_path(const char *parent, const char *child) {
    size_t parent_len = strlen(parent);
    size_t child_len = strlen(child);

    if (parent_len > (size_t)-1 - child_len - 2) {
        errno = ENAMETOOLONG;
        return NULL;
    }

    char *fullpath = malloc(parent_len + child_len + 2);
    if (!fullpath) {
        return NULL;
    }

    memcpy(fullpath, parent, parent_len);
    fullpath[parent_len] = '/';
    memcpy(fullpath + parent_len + 1, child, child_len + 1);
    return fullpath;
}

int remove_recursive(const char *path) {
    struct stat st;

    if (lstat(path, &st) != 0) {
        fprintf(stderr, "\033[38;5;196mdeldir: cannot access '%s': %s\033[0m\n", path, strerror(errno));
        return -1;
    }

    if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(path);
        if (!dir) {
            fprintf(stderr, "\033[38;5;196mdeldir: cannot open directory '%s': %s\033[0m\n", path, strerror(errno));
            return -1;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char *fullpath = join_path(path, entry->d_name);
            if (!fullpath) {
                fprintf(stderr, "\033[38;5;196mdeldir: cannot build path for '%s/%s': %s\033[0m\n",
                        path, entry->d_name, strerror(errno));
                closedir(dir);
                return -1;
            }

            if (remove_recursive(fullpath) != 0) {
                free(fullpath);
                closedir(dir);
                return -1;
            }
            free(fullpath);
        }

        closedir(dir);

        if (rmdir(path) != 0) {
            fprintf(stderr, "\033[38;5;196mdeldir: failed to remove directory '%s': %s\033[0m\n", path, strerror(errno));
            return -1;
        }

    } else {
        if (unlink(path) != 0) {
            fprintf(stderr, "\033[38;5;196mdeldir: failed to remove file '%s': %s\033[0m\n", path, strerror(errno));
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
