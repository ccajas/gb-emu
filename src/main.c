/*
#include <stdlib.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>*/

#include <stdio.h>
#include <time.h>
#include "gb.h"

#define MIN_APP

void app_setup() {}
void app_run()   {}
void app_free()  {}

int main (int argc, char * argv[])
{
    LOG_("Hello! This is GB-Emu.\n");
#ifdef GB_DEBUG
    printf("ArgCount: %d\n", argc);
#endif
    if (argc < 2)
        gb_init (&GB, "");
    else
        gb_init (&GB, argv[1]);

    const uint32_t totalFrames = 6000;
    const float totalSeconds = (float)totalFrames / 60.0;

    /* Start clock */
    clock_t t;
    t = clock();

    uint32_t i;
    for (i = 0; i < totalFrames; i++)
        gb_frame (&GB);

    gb_shutdown (&GB);

    t = clock() - t;
    double timeTaken = ((double)t)/CLOCKS_PER_SEC; /* Elapsed time */
    printf("The program took %f seconds to execute %d frames.\nGB performance is %.2f times as fast.\n", timeTaken, totalFrames, totalSeconds / timeTaken);
    printf("For each second, there is on average %.2f milliseconds free for overhead.", 1000 - (1.0f / (totalSeconds / timeTaken) * 1000));

    LOG_("Ran CPU. (%lld clocks)\n", GB.cpu.clock_m);

    return 0;
}