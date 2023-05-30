#include "app.h"

int main (int argc, char * argv[])
{
    struct App app;

    app_config (&app, argc, argv);
    app_init (&app);
    app_run (&app);

    return 0;
}