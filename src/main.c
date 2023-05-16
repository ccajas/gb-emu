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
    cpu_clock();
    gb_unload_cart(&GB);

    return 0;
}