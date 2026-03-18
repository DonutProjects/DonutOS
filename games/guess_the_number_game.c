#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

void play_game();

int main() {
    srand(time(NULL));
    
    while (1) {
        play_game();

        char choice;
        printf("\nDo you want to play again? (y/n): ");
        fflush(stdout);
        scanf(" %c", &choice);

        if (tolower(choice) != 'y') {
            printf("Exiting the game. Goodbye!\n");
            break;
        }
    }

    return 0;
}

void play_game() {
    int secret, guess, attempts = 0;
    secret = rand() % 100 + 1;

    printf("\nI've picked a number between 1 and 100.\n");
    printf("Commands:\n");
    printf("  -1  : Restart this game\n");
    printf("   0  : Quit this game\n");
    printf("  1-100: Make a guess\n\n");

    while (1) {
        printf("Your guess: ");
        fflush(stdout);
        
        if (scanf("%d", &guess) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            printf("Invalid input! Please enter a number.\n");
            continue;
        }

        if (guess == 0) {
            printf("\nExiting the game. Goodbye!\n");
            exit(0);
        }

        if (guess == -1) {
            printf("Restarting game...\n");
            return;
        }

        if (guess < 1 || guess > 100) {
            printf("Number must be between 1-100!\n");
            continue;
        }

        attempts++;

        if (guess < secret) {
            printf("Too low!\n");
        } else if (guess > secret) {
            printf("Too high!\n");
        } else {
            printf("\nCorrect! The number was %d.\n", secret);
            printf("Total attempts: %d\n", attempts);
            return;
        }
    }
}
