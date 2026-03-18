#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../gfunc.h"

#define MAX_INPUT 1024
char input[MAX_INPUT];
#define MAX_ARGS 64
char *args[MAX_ARGS];

int main() {
    while (1) {
        ClearConsole();
        printf("Choose a game:\n");
        printf("1: Guess the number\n");
        printf("2: Tic-tac-toe\n");
        //printf("3: Game 2048\n");
        //printf("4: Snake\n");
        printf("Item number>> ");
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

        int choice = atoi(args[0]);

        if (choice == 1) {
            ClearConsole();
            execl("/games/guess_the_number_game", "guess_the_number_game", (char *)NULL);
            perror("exec failed");
            break;
        } else if (choice == 2) {
            ClearConsole();
            execl("/games/tic_tac_toe_game", "tic_tac_toe_game", (char *)NULL);
            perror("exec failed");
            break;
        //} else if (choice == 3) {
        //    printf("Game 2048 not implemented yet\n");
        //    sleep(1);
        //} else if (choice == 4) {
        //    printf("Snake not implemented yet\n");
        //    sleep(1);
        } else {
            printf("Unknown choice. Try again.\n");
            sleep(1);
        }
        
    }
    return 0;
}
