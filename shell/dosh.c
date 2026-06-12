#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "../settings.h"

#define MAX_INPUT 1024
#define MAX_ARGS 64

char input[MAX_INPUT];
char parsed_input[MAX_INPUT + MAX_ARGS];
char *args[MAX_ARGS];

typedef struct Redirections {
    char *input;
    char *output;
    int append;
} Redirections;

static int parse_args(char *line, char **argv, int max_args) {
    int argc = 0;
    char *src = line;
    char *dst = parsed_input;
    char *end = parsed_input + sizeof(parsed_input);

    while (*src != '\0') {
        while (isspace((unsigned char)*src)) {
            src++;
        }
        if (*src == '\0') {
            break;
        }

        if (argc >= max_args - 1) {
            fprintf(stderr, "\033[38;5;196mDoSH: too many arguments\033[0m\n");
            return -1;
        }

        argv[argc++] = dst;

        if (*src == '<' || *src == '>') {
            if (dst + 3 > end) {
                fprintf(stderr, "\033[38;5;196mDoSH: input is too long\033[0m\n");
                return -1;
            }

            *dst++ = *src++;
            if (dst[-1] == '>' && *src == '>') {
                *dst++ = *src++;
            }
            *dst++ = '\0';
            continue;
        }

        char quote = '\0';

        while (*src != '\0') {
            if (quote != '\0') {
                if (*src == quote) {
                    quote = '\0';
                    src++;
                } else if (*src == '\\' && quote == '"' && src[1] != '\0') {
                    src++;
                    if (dst + 2 > end) {
                        fprintf(stderr, "\033[38;5;196mDoSH: input is too long\033[0m\n");
                        return -1;
                    }
                    *dst++ = *src++;
                } else {
                    if (dst + 2 > end) {
                        fprintf(stderr, "\033[38;5;196mDoSH: input is too long\033[0m\n");
                        return -1;
                    }
                    *dst++ = *src++;
                }
            } else if (*src == '\'' || *src == '"') {
                quote = *src++;
            } else if (isspace((unsigned char)*src)) {
                src++;
                break;
            } else if (*src == '<' || *src == '>') {
                break;
            } else if (*src == '\\' && src[1] != '\0') {
                src++;
                if (dst + 2 > end) {
                    fprintf(stderr, "\033[38;5;196mDoSH: input is too long\033[0m\n");
                    return -1;
                }
                *dst++ = *src++;
            } else {
                if (dst + 2 > end) {
                    fprintf(stderr, "\033[38;5;196mDoSH: input is too long\033[0m\n");
                    return -1;
                }
                *dst++ = *src++;
            }
        }

        if (quote != '\0') {
            fprintf(stderr, "\033[38;5;196mDoSH: unmatched quote\033[0m\n");
            return -1;
        }

        *dst = '\0';
        dst++;
    }

    argv[argc] = NULL;
    return argc;
}

static int parse_redirections(char **argv, Redirections *redir) {
    int out = 0;

    redir->input = NULL;
    redir->output = NULL;
    redir->append = 0;

    for (int i = 0; argv[i] != NULL; i++) {
        char *arg = argv[i];
        char *target = NULL;

        if (strcmp(arg, "<") == 0) {
            target = argv[++i];
            if (!target) {
                fprintf(stderr, "\033[38;5;196mDoSH: missing file after <\033[0m\n");
                return -1;
            }
            if (redir->input) {
                fprintf(stderr, "\033[38;5;196mDoSH: duplicate input redirection\033[0m\n");
                return -1;
            }
            redir->input = target;
        } else if (arg[0] == '<' && arg[1] != '\0') {
            if (redir->input) {
                fprintf(stderr, "\033[38;5;196mDoSH: duplicate input redirection\033[0m\n");
                return -1;
            }
            redir->input = arg + 1;
        } else if (strcmp(arg, ">") == 0 || strcmp(arg, ">>") == 0) {
            int append = strcmp(arg, ">>") == 0;
            target = argv[++i];
            if (!target) {
                fprintf(stderr, "\033[38;5;196mDoSH: missing file after %s\033[0m\n", arg);
                return -1;
            }
            if (redir->output) {
                fprintf(stderr, "\033[38;5;196mDoSH: duplicate output redirection\033[0m\n");
                return -1;
            }
            redir->output = target;
            redir->append = append;
        } else if (arg[0] == '>' && arg[1] != '\0') {
            int append = arg[1] == '>';
            target = arg + (append ? 2 : 1);
            if (*target == '\0') {
                fprintf(stderr, "\033[38;5;196mDoSH: missing file after %s\033[0m\n", append ? ">>" : ">");
                return -1;
            }
            if (redir->output) {
                fprintf(stderr, "\033[38;5;196mDoSH: duplicate output redirection\033[0m\n");
                return -1;
            }
            redir->output = target;
            redir->append = append;
        } else {
            argv[out++] = arg;
        }
    }

    argv[out] = NULL;
    return out;
}

static int apply_redirections(const Redirections *redir) {
    if (redir->input) {
        int fd = open(redir->input, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "DoSH: cannot open '%s': %s\n", redir->input, strerror(errno));
            return -1;
        }
        if (dup2(fd, STDIN_FILENO) < 0) {
            fprintf(stderr, "DoSH: cannot redirect stdin: %s\n", strerror(errno));
            close(fd);
            return -1;
        }
        close(fd);
    }

    if (redir->output) {
        int flags = O_WRONLY | O_CREAT | (redir->append ? O_APPEND : O_TRUNC);
        int fd = open(redir->output, flags, 0644);
        if (fd < 0) {
            fprintf(stderr, "DoSH: cannot open '%s': %s\n", redir->output, strerror(errno));
            return -1;
        }
        if (dup2(fd, STDOUT_FILENO) < 0) {
            fprintf(stderr, "DoSH: cannot redirect stdout: %s\n", strerror(errno));
            close(fd);
            return -1;
        }
        close(fd);
    }

    return 0;
}

int main() {
    mkdir(DONUTOS_HOME, 0755);
    if (chdir(DONUTOS_HOME) != 0) {
        perror("cd");
    }
    
    while (1) {
        printf("DonutOS> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0) continue;

        int argc = parse_args(input, args, MAX_ARGS);
        if (argc <= 0) continue;

        Redirections redir;
        argc = parse_redirections(args, &redir);
        if (argc < 0) continue;
        if (argc == 0) {
            fprintf(stderr, "\033[38;5;196mDoSH: missing command\033[0m\n");
            continue;
        }

        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                if (chdir(DONUTOS_HOME) != 0) {
                    perror("cd");
                }
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
            if (apply_redirections(&redir) != 0) {
                exit(EXIT_FAILURE);
            }
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
