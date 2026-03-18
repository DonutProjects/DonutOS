#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "../settings.h"

#define MAX_INPUT 1024
char input[MAX_INPUT];

#define MAX_ARGS 64
char *args[MAX_ARGS];

int main() {
    mkdir(DONUTOS_HOME, 0755);
    chdir(DONUTOS_HOME);
    
    while (1) {
        printf("DonutOS> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0) continue;

        int i = 0;
        char *token = strtok(input, " ");
        while (token != NULL && i < MAX_ARGS - 1) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                chdir(DONUTOS_HOME);
            } else {
                if (chdir(args[1]) != 0) {
                    perror("cd");
                }
            }
            continue;
        }

        if (strcmp(args[0], "mkdirc") == 0) {
            if (args[1] == NULL) {
                printf("\033[38;5;201mUsage: mkdirc <directory_name>\033[0m\n");
            } else {
                if (mkdir(args[1], 0755) != 0) {
                    perror("mkdir");
                }
                if (chdir(args[1]) != 0) {
                    perror("cd");
                }
            }
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            execvp(args[0], args);
            perror("DoSH");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            wait(NULL);
        } else {
            perror("DoSH");
        }
    }
    return 0;
}
