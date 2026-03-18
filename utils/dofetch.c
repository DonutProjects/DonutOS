#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include "../settings.h"

static void print_uptime(void) {
    FILE *f = fopen("/proc/uptime", "r");
    double uptime;

    if (f && fscanf(f, "%lf", &uptime) == 1) {
        int hours = (int)uptime / 3600;
        int minutes = ((int)uptime % 3600) / 60;

        char *hs = (hours == 1) ? "" : "s";
        char *ms = (minutes == 1) ? "" : "s";

        printf("Uptime: %d hour%s, %d min%s\n", hours, hs, minutes, ms);
        fclose(f);
    } else {
        printf("Uptime: N/A\n");
    }
}

static void print_os(void) {
    struct utsname sys;
    if (uname(&sys) != 0) {
        printf("OS: \033[38;5;201mDonutOS\033[0m %s (N/A)\n", DONUTOS_VERSION);
        return;
    }
    printf("OS: \033[38;5;201mDonutOS\033[0m %s (%s)\n", DONUTOS_VERSION, sys.machine);
}

static void print_cpu(void) {
    FILE *f = fopen("/proc/cpuinfo", "r");
    char line[256];
    char *model = NULL;
    int cores = 0;

    if (!f) {
        printf("CPU: N/A\n");
        return;
    }

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "model name", 10) == 0 && !model) {
            char *colon = strchr(line, ':');
            if (colon && *(colon + 1) != '\0') {
                while (*(colon + 1) == ' ') {
                    colon++;
                }
                model = strdup(colon + 1);
            }
        }
        if (strncmp(line, "processor", 9) == 0)
            cores++;
    }
    fclose(f);

    if (model) {
        model[strcspn(model, "\n")] = '\0';
        printf("CPU: %s (%d cores)\n", model, cores);
        free(model);
    } else {
        printf("CPU: N/A\n");
    }
}

static void print_mem(void) {
    long total = 0, avail = 0;
    char key[32];
    long val;
    FILE *f = fopen("/proc/meminfo", "r");

    if (!f) {
        printf("Memory: N/A\n");
        return;
    }

    while (fscanf(f, "%31s %ld kB", key, &val) == 2) {
        if (strcmp(key, "MemTotal:") == 0) total = val;
        if (strcmp(key, "MemAvailable:") == 0) avail = val;
    }
    fclose(f);

    if (total <= 0) {
        printf("Memory: N/A\n");
        return;
    }

    if (avail < 0) {
        avail = 0;
    } else if (avail > total) {
        avail = total;
    }
    printf("Memory: %ldMiB / %ldMiB\n", (total - avail) / 1024, total / 1024);
}

int main() {
    print_uptime();
    print_os();
    print_cpu();
    print_mem();
    return 0;
}
