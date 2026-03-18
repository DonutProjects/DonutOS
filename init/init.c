#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/reboot.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "../settings.h"
#include "../gfunc.h"

static volatile sig_atomic_t power_request = 0;

static void power(int signo) {
    power_request = signo;
}

static void handle_power_request(void) {
    int signo = power_request;
    if (signo == 0) {
        return;
    }
    power_request = 0;

    if (signo == SIGUSR1) {
        printf("\033[38;5;201mSystem shutdown in progress...\033[0m\n");
    } else if (signo == SIGUSR2) {
        printf("\033[38;5;201mThe system is rebooting...\033[0m\n");
    }

    kill(-1, SIGTERM);
    usleep(200000);
    kill(-1, SIGKILL);
    sync();

    if (signo == SIGUSR1) {
        if (reboot(RB_POWER_OFF) != 0) {
            fprintf(stderr, "\033[38;5;196mError while shutting down: \033[0m%s\n", strerror(errno));
        }
        exit(EXIT_SUCCESS);
    } else if (signo == SIGUSR2) {
        if (reboot(RB_AUTOBOOT) != 0) {
            fprintf(stderr, "\033[38;5;196mError during reboot: \033[0m%s\n", strerror(errno));
        }
        exit(EXIT_SUCCESS);
    }
}

static void sigchld_handler(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0) { }
}

static void sigs(void) {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa_shutdown;
    sa_shutdown.sa_handler = power;
    sigemptyset(&sa_shutdown.sa_mask);
    sa_shutdown.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa_shutdown, NULL) == -1) {
        perror("sigaction SIGUSR1");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa_reboot;
    sa_reboot.sa_handler = power;
    sigemptyset(&sa_reboot.sa_mask);
    sa_reboot.sa_flags = 0;
    if (sigaction(SIGUSR2, &sa_reboot, NULL) == -1) {
        perror("sigaction SIGUSR2");
        exit(EXIT_FAILURE);
    }
}

static void mount_filesystems(void) {
    mkdir("/proc", 0555);
    if (mount("proc", "/proc", "proc", 0, NULL) < 0) {
        fprintf(stderr, "\n\033[38;5;196mError: error mounting /proc: \033[0m%s\n", strerror(errno));
    }

    mkdir("/sys", 0555);
    if (mount("sysfs", "/sys", "sysfs", 0, NULL) < 0) {
        fprintf(stderr, "\n\033[38;5;196mError: error mounting /sys: \033[0m%s\n", strerror(errno));
    }

    mkdir("/dev", 0755);
    if (mount("devtmpfs", "/dev", "devtmpfs", 0, NULL) < 0) {
        fprintf(stderr, "\n\033[38;5;196mError: error mounting /dev: \033[0m%s\n", strerror(errno));
    }
}

int main() {
    if (getpid() != 1) {
        fprintf(stderr, "Error: The init system must run as PID 1.\n");
        exit(EXIT_FAILURE);
    }

    ClearConsole();
    printf("\033[0mWelcome to DonutOS %s\n", DONUTOS_VERSION);
    DrawDonut();

    printf("\rLoading...");
    fflush(stdout);

    sigs();

    mount_filesystems();

    printf(" \033[38;5;45mDone!\033[0m\n");

    printf("\033[38;5;46mStarting...\033[0m\n");

    while (1) {
        handle_power_request();

        pid_t pid = fork();
        if (pid == 0) {
            setsid();
            execl("/bin/dosh", "dosh", NULL);
            perror("execl");
            exit(1);
        } else if (pid < 0) {
            perror("fork");
            sleep(1);
            continue;
        }

        int status;
        while (waitpid(pid, &status, 0) < 0) {
            if (errno == EINTR) {
                handle_power_request();
                continue;
            }
            perror("waitpid");
            break;
        }
    }

    return 0;
}
