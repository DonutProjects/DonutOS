#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Editor {
    char **lines;
    size_t count;
    size_t cap;
    char *filename;
    bool dirty;
    bool final_newline;
} Editor;

static void free_editor(Editor *ed) {
    for (size_t i = 0; i < ed->count; i++) {
        free(ed->lines[i]);
    }
    free(ed->lines);
    free(ed->filename);
}

static char *dup_string(const char *s) {
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (!copy) {
        return NULL;
    }
    memcpy(copy, s, len);
    return copy;
}

static bool grow_lines(Editor *ed, size_t needed) {
    if (needed <= ed->cap) {
        return true;
    }

    size_t new_cap = ed->cap ? ed->cap * 2 : 16;
    while (new_cap < needed) {
        if (new_cap > (size_t)-1 / 2) {
            fprintf(stderr, "txted: file is too large\n");
            return false;
        }
        new_cap *= 2;
    }
    if (new_cap > (size_t)-1 / sizeof(ed->lines[0])) {
        fprintf(stderr, "txted: file is too large\n");
        return false;
    }

    char **new_lines = realloc(ed->lines, new_cap * sizeof(ed->lines[0]));
    if (!new_lines) {
        fprintf(stderr, "txted: out of memory\n");
        return false;
    }

    ed->lines = new_lines;
    ed->cap = new_cap;
    return true;
}

static bool insert_line(Editor *ed, size_t at, const char *text) {
    if (at > ed->count) {
        at = ed->count;
    }

    if (!grow_lines(ed, ed->count + 1)) {
        return false;
    }

    char *copy = dup_string(text);
    if (!copy) {
        fprintf(stderr, "txted: out of memory\n");
        return false;
    }

    memmove(&ed->lines[at + 1], &ed->lines[at], (ed->count - at) * sizeof(ed->lines[0]));
    ed->lines[at] = copy;
    ed->count++;
    ed->dirty = true;
    ed->final_newline = true;
    return true;
}

static bool replace_line(Editor *ed, size_t at, const char *text) {
    if (at >= ed->count) {
        return false;
    }

    char *copy = dup_string(text);
    if (!copy) {
        fprintf(stderr, "txted: out of memory\n");
        return false;
    }

    free(ed->lines[at]);
    ed->lines[at] = copy;
    ed->dirty = true;
    ed->final_newline = true;
    return true;
}

static bool delete_range(Editor *ed, size_t first, size_t last) {
    if (first >= ed->count || last >= ed->count || first > last) {
        return false;
    }

    for (size_t i = first; i <= last; i++) {
        free(ed->lines[i]);
    }

    size_t remaining = ed->count - last - 1;
    memmove(&ed->lines[first], &ed->lines[last + 1], remaining * sizeof(ed->lines[0]));
    ed->count -= last - first + 1;
    ed->dirty = true;
    ed->final_newline = ed->count > 0;
    return true;
}

static void strip_newline(char *line, bool *had_newline) {
    size_t len = strlen(line);
    *had_newline = false;

    if (len > 0 && line[len - 1] == '\n') {
        line[--len] = '\0';
        *had_newline = true;
    }
    if (len > 0 && line[len - 1] == '\r') {
        line[--len] = '\0';
    }
}

static bool load_file(Editor *ed, const char *filename) {
    ed->filename = dup_string(filename);
    if (!ed->filename) {
        fprintf(stderr, "txted: out of memory\n");
        return false;
    }

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        if (errno == ENOENT) {
            printf("txted: new file '%s'\n", filename);
            return true;
        }
        fprintf(stderr, "txted: cannot open '%s': %s\n", filename, strerror(errno));
        return false;
    }

    char *line = NULL;
    size_t cap = 0;
    ssize_t nread;
    bool last_had_newline = false;

    while ((nread = getline(&line, &cap, fp)) != -1) {
        (void)nread;
        strip_newline(line, &last_had_newline);
        if (!insert_line(ed, ed->count, line)) {
            free(line);
            fclose(fp);
            return false;
        }
    }

    if (ferror(fp)) {
        fprintf(stderr, "txted: cannot read '%s': %s\n", filename, strerror(errno));
        free(line);
        fclose(fp);
        return false;
    }

    free(line);
    fclose(fp);
    ed->dirty = false;
    ed->final_newline = ed->count > 0 && last_had_newline;
    printf("txted: loaded %zu line%s from '%s'\n", ed->count, ed->count == 1 ? "" : "s", filename);
    return true;
}

static bool save_file(Editor *ed) {
    FILE *fp = fopen(ed->filename, "w");
    if (!fp) {
        fprintf(stderr, "txted: cannot save '%s': %s\n", ed->filename, strerror(errno));
        return false;
    }

    for (size_t i = 0; i < ed->count; i++) {
        if (fputs(ed->lines[i], fp) == EOF) {
            fprintf(stderr, "txted: cannot write '%s': %s\n", ed->filename, strerror(errno));
            fclose(fp);
            return false;
        }

        if (i + 1 < ed->count || ed->final_newline) {
            if (fputc('\n', fp) == EOF) {
                fprintf(stderr, "txted: cannot write '%s': %s\n", ed->filename, strerror(errno));
                fclose(fp);
                return false;
            }
        }
    }

    if (fclose(fp) != 0) {
        fprintf(stderr, "txted: cannot close '%s': %s\n", ed->filename, strerror(errno));
        return false;
    }

    ed->dirty = false;
    printf("txted: saved %zu line%s to '%s'\n", ed->count, ed->count == 1 ? "" : "s", ed->filename);
    return true;
}

static char *skip_space(char *s) {
    while (isspace((unsigned char)*s)) {
        s++;
    }
    return s;
}

static bool parse_number(char **cursor, size_t *value) {
    char *s = skip_space(*cursor);
    if (!isdigit((unsigned char)*s)) {
        return false;
    }

    errno = 0;
    char *end = NULL;
    unsigned long parsed = strtoul(s, &end, 10);
    if (errno != 0 || end == s || parsed == 0) {
        return false;
    }

    *value = (size_t)parsed;
    *cursor = end;
    return true;
}

static void print_help(void) {
    puts("Commands:");
    puts("  l [first [last]]  list lines");
    puts("  a [after]         append lines after number; end with a single .");
    puts("  i [before]        insert lines before number; end with a single .");
    puts("  r <line> <text>   replace one line");
    puts("  d <first> [last]  delete line or range");
    puts("  w                 write file");
    puts("  q                 quit; asks again if there are unsaved changes");
    puts("  q!                quit without saving");
    puts("  h                 show this help");
}

static void list_lines(const Editor *ed, size_t first, size_t last) {
    if (ed->count == 0) {
        puts("[empty]");
        return;
    }

    if (first < 1) {
        first = 1;
    }
    if (last < first || last > ed->count) {
        last = ed->count;
    }

    for (size_t i = first; i <= last; i++) {
        printf("%4zu  %s\n", i, ed->lines[i - 1]);
    }
}

static bool read_insert_block(Editor *ed, size_t at) {
    char *line = NULL;
    size_t cap = 0;

    puts("Enter text. A single . on a line ends input.");
    while (1) {
        printf("] ");
        fflush(stdout);

        ssize_t nread = getline(&line, &cap, stdin);
        if (nread == -1) {
            free(line);
            putchar('\n');
            return false;
        }

        bool had_newline;
        strip_newline(line, &had_newline);
        (void)had_newline;

        if (strcmp(line, ".") == 0) {
            free(line);
            return true;
        }

        if (!insert_line(ed, at, line)) {
            free(line);
            return false;
        }
        at++;
    }
}

static bool confirm_dirty_quit(Editor *ed) {
    if (!ed->dirty) {
        return true;
    }

    puts("txted: unsaved changes. Type q again to quit, or w to save.");
    printf("txted> ");
    fflush(stdout);

    char *line = NULL;
    size_t cap = 0;
    if (getline(&line, &cap, stdin) == -1) {
        free(line);
        putchar('\n');
        return false;
    }
    bool had_newline;
    strip_newline(line, &had_newline);
    (void)had_newline;

    char *cmd = skip_space(line);
    bool quit = strcmp(cmd, "q") == 0;
    bool save = strcmp(cmd, "w") == 0;
    free(line);

    if (save) {
        return save_file(ed);
    }
    return quit;
}

static bool run_command(Editor *ed, char *line, bool *running) {
    char *cmd = skip_space(line);
    if (*cmd == '\0') {
        return true;
    }

    char op = *cmd++;
    if (op == 'h' || op == '?') {
        print_help();
        return true;
    }

    if (op == 'l' || op == 'p') {
        size_t first = 1;
        size_t last = ed->count;
        char *args = cmd;
        if (parse_number(&args, &first)) {
            last = first;
            parse_number(&args, &last);
        }
        list_lines(ed, first, last);
        return true;
    }

    if (op == 'a') {
        size_t after = ed->count;
        char *args = cmd;
        if (parse_number(&args, &after) && after > ed->count) {
            printf("txted: line %zu does not exist\n", after);
            return true;
        }
        return read_insert_block(ed, after);
    }

    if (op == 'i') {
        size_t before = ed->count + 1;
        char *args = cmd;
        if (parse_number(&args, &before) && before > ed->count + 1) {
            printf("txted: line %zu does not exist\n", before);
            return true;
        }
        if (before == 0) {
            before = 1;
        }
        return read_insert_block(ed, before - 1);
    }

    if (op == 'r') {
        size_t line_no;
        char *args = cmd;
        if (!parse_number(&args, &line_no) || line_no > ed->count) {
            puts("txted: usage: r <line> <text>");
            return true;
        }

        char *text = skip_space(args);
        if (!replace_line(ed, line_no - 1, text)) {
            puts("txted: replace failed");
            return false;
        }
        return true;
    }

    if (op == 'd') {
        size_t first;
        size_t last;
        char *args = cmd;
        if (!parse_number(&args, &first)) {
            puts("txted: usage: d <first> [last]");
            return true;
        }
        last = first;
        parse_number(&args, &last);

        if (first > ed->count || last > ed->count || first > last) {
            puts("txted: invalid line range");
            return true;
        }

        return delete_range(ed, first - 1, last - 1);
    }

    if (op == 'w') {
        return save_file(ed);
    }

    if (op == 'q') {
        if (strcmp(skip_space(cmd), "!") == 0 || confirm_dirty_quit(ed)) {
            *running = false;
        }
        return true;
    }

    puts("txted: unknown command; type h for help");
    return true;
}

static int edit(Editor *ed) {
    char *line = NULL;
    size_t cap = 0;
    bool running = true;

    print_help();
    while (running) {
        printf("txted%s> ", ed->dirty ? "*" : "");
        fflush(stdout);

        ssize_t nread = getline(&line, &cap, stdin);
        if (nread == -1) {
            putchar('\n');
            if (ed->dirty) {
                puts("txted: unsaved changes were not written");
                free(line);
                return 1;
            }
            break;
        }

        bool had_newline;
        strip_newline(line, &had_newline);
        (void)nread;
        (void)had_newline;

        if (!run_command(ed, line, &running)) {
            free(line);
            return 1;
        }
    }

    free(line);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "\033[38;5;201mUsage: %s FILE\033[0m\n", argv[0]);
        return 1;
    }

    Editor ed = {0};
    if (!load_file(&ed, argv[1])) {
        free_editor(&ed);
        return 1;
    }

    int status = edit(&ed);
    free_editor(&ed);
    return status;
}
