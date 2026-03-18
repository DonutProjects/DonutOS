#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#define SIZE 3

typedef struct { int row, col; } Move;

static void init_board(char b[SIZE][SIZE]) {
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j)
            b[i][j] = ' ';
}

static void print_board(const char b[SIZE][SIZE]) {
    printf("\n    1   2   3\n");
    for (int i = 0; i < SIZE; ++i) {
        printf(" %d  ", i + 1);
        for (int j = 0; j < SIZE; ++j) {
            printf("%c", b[i][j]);
            if (j < SIZE - 1) printf(" | ");
        }
        printf("\n");
        if (i < SIZE - 1) printf("   ---+---+---\n");
    }
    printf("\n");
}

static bool is_moves_left(const char b[SIZE][SIZE]) {
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j)
            if (b[i][j] == ' ') return true;
    return false;
}

static char check_winner(const char b[SIZE][SIZE]) {
    for (int i = 0; i < SIZE; ++i) {
        if (b[i][0] != ' ' && b[i][0] == b[i][1] && b[i][1] == b[i][2]) return b[i][0];
        if (b[0][i] != ' ' && b[0][i] == b[1][i] && b[1][i] == b[2][i]) return b[0][i];
    }
    if (b[0][0] != ' ' && b[0][0] == b[1][1] && b[1][1] == b[2][2]) return b[0][0];
    if (b[0][2] != ' ' && b[0][2] == b[1][1] && b[1][1] == b[2][0]) return b[0][2];

    return is_moves_left(b) ? ' ' : 'D';
}

static int score_result(char winner, int depth, char ai, char human) {
    if (winner == ai)    return 10 - depth;
    if (winner == human) return depth - 10;
    return 0;
}

static int minimax(char b[SIZE][SIZE], int depth, bool isMax, char ai, char human) {
    char winner = check_winner(b);
    if (winner != ' ') return score_result(winner, depth, ai, human);

    if (isMax) {
        int best = -10000;
        for (int i = 0; i < SIZE; ++i)
            for (int j = 0; j < SIZE; ++j)
                if (b[i][j] == ' ') {
                    b[i][j] = ai;
                    int val = minimax(b, depth + 1, false, ai, human);
                    b[i][j] = ' ';
                    if (val > best) best = val;
                }
        return best;
    } else {
        int best = 10000;
        for (int i = 0; i < SIZE; ++i)
            for (int j = 0; j < SIZE; ++j)
                if (b[i][j] == ' ') {
                    b[i][j] = human;
                    int val = minimax(b, depth + 1, true, ai, human);
                    b[i][j] = ' ';
                    if (val < best) best = val;
                }
        return best;
    }
}

static Move find_best_move(char b[SIZE][SIZE], char ai, char human) {
    int bestVal = -10000;
    Move best = { -1, -1 };

    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            if (b[i][j] == ' ') {
                b[i][j] = ai;
                int moveVal = minimax(b, 0, false, ai, human);
                b[i][j] = ' ';
                if (moveVal > bestVal) {
                    bestVal = moveVal;
                    best.row = i; best.col = j;
                }
            }
        }
    }
    return best;
}

static char first_char(const char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    return *s ? *s : '\0';
}

static int prompt_first_player(void) {
    char line[64];
    while (1) {
        printf("=== Tic-Tac-Toe (Human vs AI) ===\n");
        printf("Who goes first?\n");
        printf("1) Me (I'll be 'X')\n");
        printf("2) Computer (I'll be 'O')\n");
        printf("Type 1/2, or 'q' to quit: ");
        if (!fgets(line, sizeof line, stdin)) return -1;
        char ch = first_char(line);
        if (ch == '\0') continue;
        if (ch == '1') return 1;
        if (ch == '2') return 2;
        if (ch == 'q' || ch == 'Q') return -1;
        printf("Invalid choice. Try again.\n\n");
    }
}

static int read_human_move(char b[SIZE][SIZE], char human) {
    char line[64];
    int r = 0, c = 0;
    while (1) {
        printf("Your move (%c). Enter row and column (1-3 1-3), 'r' to restart, or 'q' to quit: ", human);
        if (!fgets(line, sizeof line, stdin)) return -1;
        char ch = first_char(line);
        if (ch == '\0') continue;
        if (ch == 'q' || ch == 'Q') return -1;
        if (ch == 'r' || ch == 'R') return 0;

        if (sscanf(line, "%d %d", &r, &c) != 2) {
            printf("Invalid input. Example: 2 3\n");
            continue;
        }
        if (r < 1 || r > 3 || c < 1 || c > 3) {
            printf("Out of range. Use numbers 1..3.\n");
            continue;
        }
        if (b[r-1][c-1] != ' ') {
            printf("Cell is occupied. Try again.\n");
            continue;
        }
        b[r-1][c-1] = human;
        return 1;
    }
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

    char board[SIZE][SIZE];
    bool running = true;

    while (running) {
        int choice = prompt_first_player();
        if (choice == -1) break;

        char human = (choice == 2) ? 'O' : 'X';
        char ai    = (human == 'X') ? 'O' : 'X';
        char turn  = 'X';

        init_board(board);
        printf("\nTips: type 'r' to restart current game, 'q' to quit anytime.\n");

        while (1) {
            print_board(board);

            if (turn == human) {
                int res = read_human_move(board, human);
                if (res == -1) { running = false; printf("Goodbye!\n"); break; }
                if (res == 0) { init_board(board); turn = 'X'; printf("Game restarted.\n"); continue; }
            } else {
                Move m = find_best_move(board, ai, human);
                if (m.row >= 0 && m.col >= 0) {
                    board[m.row][m.col] = ai;
                    printf("AI move (%c): %d %d\n", ai, m.row + 1, m.col + 1);
                }
            }

            char w = check_winner(board);
            if (w != ' ') {
                print_board(board);
                if (w == 'D') printf("Draw!\n");
                else if (w == human) printf("You win! 🎉\n");
                else printf("AI wins. 🤖\n");
                break;
            }

            turn = (turn == 'X') ? 'O' : 'X';
        }

        if (!running) break;

        while (1) {
            char line[32];
            printf("Play again? (y/n): ");
            if (!fgets(line, sizeof line, stdin)) { running = false; break; }
            char ch = first_char(line);
            if (ch == 'y' || ch == 'Y') { printf("\n"); break; }
            if (ch == 'n' || ch == 'N' || ch == 'q' || ch == 'Q') { running = false; break; }
            printf("Please answer 'y' or 'n'.\n");
        }
    }

    return 0;
}
