/*#include <stdio.h>
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
    clock_t t;
    t = clock();

    if (argc < 2)
        gb_init (&GB, "");
    else
        gb_init (&GB, argv[1]);        
    gb_run (&GB);
    gb_shutdown (&GB);

    t = clock() - t;
    double time_taken = ((double)t)/CLOCKS_PER_SEC; // calculate the elapsed time
    printf("The program took %f seconds to execute.\nGB performance is %.2f times as fast.\n", time_taken, GB.seconds / time_taken);
    printf("For each second, there is on average %.2f milliseconds free for overhead.", 1000 - (1.0f / (GB.seconds / time_taken) * 1000));

    LOG_("Ran CPU. (%lld clocks)\n", cpu->clock_m);

    return 0;
}