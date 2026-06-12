#include <stdio.h>
#include <string.h>

static void print_help(const char *prog) {
    printf("\033[38;5;201mUsage: %s [-n] [TEXT...]\033[0m\n", prog);
    puts("Prints TEXT to the console.");
    puts("");
    puts("Options:");
    puts("  -n        Do not print the trailing newline");
    puts("  --help     Show this help text");
}

int main(int argc, char *argv[]) {
    int newline = 1;
    int start = 1;

    if (argc > 1 && (strcmp(argv[1], "--help") == 0)) {
        print_help(argv[0]);
        return 0;
    }

    if (argc > 1 && strcmp(argv[1], "-n") == 0) {
        newline = 0;
        start = 2;
    }

    for (int i = start; i < argc; i++) {
        if (i > start) {
            putchar(' ');
        }
        fputs(argv[i], stdout);
    }

    if (newline) {
        putchar('\n');
    }

    return ferror(stdout) ? 1 : 0;
}
