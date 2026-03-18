#include <stdio.h>
#include "../settings.h"
#include "../gfunc.h"

int main() {
    DrawDonut();
    printf("\nDonutOS %s\n", DONUTOS_VERSION);
    printf("DonutOS was made by Alex (Discord: %s).\n", CREATOR_USERNAME);
    printf("The idea of making DonutOS came from %s (Discord: %s).\n", IDEA_CREATOR_USERNAME, IDEA_CREATOR_USERNAME);
    return 0;
}
