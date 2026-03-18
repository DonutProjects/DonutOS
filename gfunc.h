#include <stdio.h>

static inline void ClearConsole() {
    printf("\033[H\033[J\n");
}

static inline void DrawDonut() {
    const char* donut[] = {
        "\033[38;5;211m         .:^^^^^:::..         ",
        "     .:~~!7!~~!!^^~^::..      ",
        "   .^7??777!!!!!!~~~^^^^:.    ",
        "  ^?????7?!7?JJJJ77!!~!^^::.  ",
        " ~J????7777??J55P5Y7777!~~::. ",
        "^YYJJ777!~!7J?JYPGPYJ7!7~~^::.",
        "?5Y?\?\?!!!~77.   .~PGPY~!!!~^^:",
        "J5YY?\?!~~~7:      ?G5J7777!~^:",
        "7P5J?\?\?!!~~~.   .^Y5J?77!~~~^:",
        ":5PYYY?7!~~~~^^!7JJ?\?\?\?!7!~~^.",
        " ~PP5YY?7!777~~!77?777!77!!!: ",
        "  ~5GPYYJ?J???77?????7~7!!~.  ",
        "   :?PGP55Y??JJ?JJJJJJ??7^    ",
        "     :7Y5PG5JY55YJJJYJ7~.     ",
        "        :~7?YJYJJ?!!^.        \033[0m"
    };

    int lines = sizeof(donut) / sizeof(donut[0]);
    for (int i = 0; i < lines; i++) {
        printf("%s\n", donut[i]);
    }
}
