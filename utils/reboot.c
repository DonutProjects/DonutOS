#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int main() {
    int sig = SIGUSR2;

    if (kill(1, sig) == -1) {
        perror("kill");
        return 1;
    }

    return 0;
}
