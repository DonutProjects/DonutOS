#include <stdio.h>

static void HelpText() {
    struct CommandHelp {
        const char* commands;
        const char* description;
    };

    struct CommandHelp help[] = {
        {"hello",         "Prints 'Hello World!' to the console"},
        {"cat",           "Outputs the contents of a file to the console"},
        {"bomb",          "Displays an ANSI bomb in the console"},
        {"donut",         "Displays an ANSI donut in the console"},
        {"dir",           "Lists all files in the current directory"},
        {"cut",           "Moves a file to the selected folder"},
        {"poweroff",      "Shuts down the computer"},
        {"reboot",        "Reboots the computer"},
        {"cd",            "Changes the current directory"},
        {"mkdir",         "Creates a new directory"},
        {"mkdirc",        "Creates a directory and changes into it"},
        {"wd",            "Prints the current directory's absolute path"},
        {"del",           "Deletes a file"},
        {"deldir",        "Deletes a directory"},
        {"clear",         "Clears the console screen"},
        {"help",          "Shows this help text"},
        {"about",         "Shows information about DonutOS"},
        {"dofetch",       "Displays system information"},
        {"games",         "Launches a list of available games"},
        {"touch",         "Creates an empty file"},
        {"txted",         "Opens the specified file in text edit mode"}
    };

    int lines = sizeof(help) / sizeof(help[0]);
    printf("\033[38;5;201mCommands:\033[0m\n\n");
    for (int i = 0; i < lines; i++) {
        printf("  %-13s - %s\n", help[i].commands, help[i].description);
    }
}

int main() {
    HelpText();
    return 0;
}
