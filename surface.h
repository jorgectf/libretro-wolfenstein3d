#ifndef SURFACE_H
#define SURFACE_H

#include <stdint.h>
#include "SDL.h"

typedef struct LR_Surface
{
   SDL_Surface *surf;
} LR_Surface;

typedef struct LR_Color
{
   uint8_t r;
   uint8_t g;
   uint8_t b;
   uint8_t unused;
} LR_Color;

#define RED_EXPAND   3
#define GREEN_EXPAND 2
#define BLUE_EXPAND  3

#define RED_SHIFT   11
#define GREEN_SHIFT 5
#define BLUE_SHIFT  0

#define SET_COLORFORMAT(r, g, b) ((r >> RED_EXPAND) << RED_SHIFT | (g >> GREEN_EXPAND) << GREEN_SHIFT | (b >> BLUE_EXPAND) << BLUE_SHIFT)

uint32_t LR_GetTicks(void);

void LR_FillRect(SDL_Surface *surface, const void *rect_data, uint32_t color);

void LR_SetPalette(SDL_Surface *surface, int flags, LR_Color *colors, int firstcolor, int ncolors);

int LR_SetColors(SDL_Surface *surface, LR_Color *colors, int firstcolor, int ncolors);

#endif