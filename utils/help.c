#include <stdarg.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

typedef struct Pager {
    int enabled;
    int page_lines;
    int printed;
} Pager;

static int terminal_rows(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0) {
        return ws.ws_row;
    }
    return 24;
}

static void pager_init(Pager *pager) {
    pager->enabled = isatty(STDOUT_FILENO) && isatty(STDIN_FILENO);
    pager->page_lines = terminal_rows() - 3;
    if (pager->page_lines < 5) {
        pager->page_lines = 5;
    }
    pager->printed = 0;
}

static int pager_pause(Pager *pager) {
    int c;

    if (!pager->enabled || pager->printed < pager->page_lines) {
        return 1;
    }

    printf("\033[7m-- More -- Enter: continue, q+Enter: quit\033[0m");
    fflush(stdout);

    do {
        c = getchar();
        if (c == 'q' || c == 'Q' || c == EOF) {
            printf("\r\033[K");
            return 0;
        }
    } while (c != '\n');

    printf("\r\033[K");
    pager->printed = 0;
    return 1;
}

static int pager_printf(Pager *pager, const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    pager->printed++;
    return pager_pause(pager);
}

static void HelpText(void) {
    Pager pager;
    struct CommandHelp {
        const char* commands;
        const char* description;
    };

    struct CommandHelp help[] = {
        {"hello",         "Prints 'Hello World!' to the console"},
        {"cat",           "Outputs the contents of a file to the console"},
        {"bomb",          "Displays an ANSI bomb in the console"},
        {"donut",         "Displays an ANSI donut in the console"},
        {"echo",          "Prints arguments to the console"},
        {"dir",           "Lists all files in the current directory"},
        {"cut",           "Moves or renames a file"},
        {"poweroff",      "Shuts down the computer"},
        {"reboot",        "Reboots the computer"},
        {"cd",            "Changes the current directory"},
        {"mkdir",         "Creates a new directory"},
        {"mkdirc",        "Creates a directory and changes into it"},
        {"wd",            "Prints the current directory's absolute path"},
        {"del",           "Deletes a file"},
        {"deldir",        "Recursively deletes files and directories"},
        {"clear",         "Clears the console screen"},
        {"help",          "Shows this help text"},
        {"about",         "Shows information about DonutOS"},
        {"dofetch",       "Displays system information"},
        {"games",         "Launches a list of available games"},
        {"touch",         "Creates an empty file"},
        {"txted",         "Opens a lightweight line editor"}
    };

    pager_init(&pager);

    int lines = sizeof(help) / sizeof(help[0]);
    if (!pager_printf(&pager, "\033[38;5;201mCommands:\033[0m\n")) return;
    if (!pager_printf(&pager, "\n")) return;
    for (int i = 0; i < lines; i++) {
        if (!pager_printf(&pager, "  %-13s - %s\n", help[i].commands, help[i].description)) return;
    }
    if (!pager_printf(&pager, "\n")) return;
    pager_printf(&pager, "Redirection: command > file, command >> file, command < file\n");
}

int main(void) {
    HelpText();
    return 0;
}
