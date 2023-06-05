#ifndef APP_TIGR_H
#define APP_TIGR_H

static void key_input (Tigr * screen)
{
    if (tigrKeyHeld(screen, TK_LEFT) || tigrKeyHeld(screen, 'A')) { }
        //playerxs -= 10;
}

#ifdef USE_TIGR
        .tileMap = tigrBitmap (128, 128),
        .frameBuffer = tigrBitmap (DISPLAY_WIDTH, DISPLAY_HEIGHT)
#endif

#ifdef USE_TIGR
    app->screen = tigrWindow(DISPLAY_WIDTH * app->scale, DISPLAY_HEIGHT * app->scale, "GB Emu", TIGR_FIXED);
#endif

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

#ifdef USE_TIGR
        while (!tigrClosed(app->screen))
        {
            tigrClear (app->screen, tigrRGB(0x80, 0x90, 0xa0));
            app_draw (app);
        }
        tigrFree (app->screen);
#endif

#endif