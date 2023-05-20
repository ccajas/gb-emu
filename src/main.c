/*#include <stdio.h>
#include <stdlib.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>*/

#include <stdio.h>
#include "gb.h"

#define MIN_APP

void app_setup() {}
void app_run()   {}
void app_free()  {}

int main (int argc, char** argv)
{
    LOG_("Hello! This is GB-Emu.\n");

    gb_reset(&GB);
    gb_load_cart(&GB);
    cpu_init();
    cpu_state();

    //while (gb_running(&GB))
    //{
        cpu_clock();
    //}
    gb_unload_cart(&GB);
    
    LOG_("Ran CPU. (%lld clocks)\n", cpu->clock_m);
    LOG_("%d%% of 256 instructions done.\n", ((256 - cpu->ni) * 100) / 256);

    return 0;
}