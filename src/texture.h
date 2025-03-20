#ifndef TEXTURE_H
#define TEXTURE_H

struct Texture
{
    uint16_t width;
    uint16_t height;

    /* Pixel data and pointer */
    uint8_t * imgData, * ptr;
};

#endif