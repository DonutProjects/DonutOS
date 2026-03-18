#include <stdio.h>
#include <unistd.h>

#define PATH_MAX 4096

int main() {
    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd");
        return 1;
    }

    return 0;
}
