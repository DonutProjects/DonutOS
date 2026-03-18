#include <stdio.h>
#include <unistd.h>
#include "../gfunc.h"

static void DrawBomb() {
    const char* bomb[] = {
        "\033[38;5;100m       ,--.!,",
        "    __/   -*-",
        "  ,d08b.  '|`",
        "  0088MM",
        "  `9MMP'\033[0m"
    };

    int lines = sizeof(bomb) / sizeof(bomb[0]);
    for (int i = 0; i < lines; i++) {
        printf("%s\n", bomb[i]);
    }
}

int main() {
    DrawBomb();
    sleep(2);
    ClearConsole();
    return 0;
}
