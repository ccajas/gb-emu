#ifndef APP_TIGR_H
#define APP_TIGR_H

static void key_input (Tigr * screen)
{
    if (tigrKeyHeld(screen, TK_ESCAPE) || tigrKeyHeld(screen, 'A')) { }
        //playerxs -= 10;
}

void app_draw (struct App * app)
{
    /* Test background fill */
    tigrClear (app->gbData.frameBuffer, tigrRGB(80, 180, 255));
    tigrFill (app->gbData.frameBuffer, 0, 100, 20, 40, tigrRGB(60, 120, 60));
    tigrFill (app->gbData.frameBuffer, 0, 500, 40, 3, tigrRGB(0, 0, 0));
    tigrLine (app->gbData.frameBuffer, 0, 101, 320, 201, tigrRGB(255, 255, 255));

    tigrBlit (app->screen, app->gbData.frameBuffer, 0, 0, 0, 0, 
        app->gbData.frameBuffer->w, app->gbData.frameBuffer->h);

    tigrPrint (app->screen, tfont, 120, 110, tigrRGB(0xff, 0xff, 0xff), "Hello, world!");

    tigrUpdate (app->screen);
}

#endif