#include <stdint.h>

#if defined(__CELLOS_LV2__) && !defined(__PSL1GHT__)
#include <sys/timer.h>
#elif defined(XENON)
#include <time/time.h>
#elif defined(GEKKO) || defined(__PSL1GHT__) || defined(__QNX__)
#include <unistd.h>
#elif defined(PSP)
#include <pspthreadman.h> 
#elif defined(VITA)
#include <psp2/kernel/threadmgr.h>
#elif defined(_3DS)
#include <3ds.h>
#else
#include <time.h>
#endif

#include "surface.h"
#include "SDL.h"

typedef uint64_t retro_perf_tick_t;

unsigned short d_8to16table[256];

/* forward decls */
int SDL_Flip(SDL_Surface* screen);
SDL_Surface *SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags);
SDL_Surface* SDL_ConvertSurface(SDL_Surface*           src,
                                const SDL_PixelFormat* fmt,
                                Uint32                 flags);
int SDL_SetPalette(SDL_Surface *surface, int flags, SDL_Color *colors, int firstcolor, int ncolors);;
int SDL_SetColors(SDL_Surface *surface, SDL_Color *colors, int firstcolor, int ncolors);

/**
 * rarch_sleep:
 * @msec         : amount in milliseconds to sleep
 *
 * Sleeps for a specified amount of milliseconds (@msec).
 **/
static inline void rarch_sleep(unsigned msec)
{
#if defined(__CELLOS_LV2__) && !defined(__PSL1GHT__)
   sys_timer_usleep(1000 * msec);
#elif defined(PSP) || defined(VITA)
   sceKernelDelayThread(1000 * msec);
#elif defined(_3DS)
   svcSleepThread(1000000 * (s64)msec);
#elif defined(_WIN32)
   Sleep(msec);
#elif defined(XENON)
   udelay(1000 * msec);
#elif defined(GEKKO) || defined(__PSL1GHT__) || defined(__QNX__)
   usleep(1000 * msec);
#else
   struct timespec tv = {0};
   tv.tv_sec = msec / 1000;
   tv.tv_nsec = (msec % 1000) * 1000000;
   nanosleep(&tv, NULL);
#endif
}

static retro_perf_tick_t rarch_get_perf_counter(void)
{
   retro_perf_tick_t time_ticks = 0;
#if defined(__linux__) || defined(__QNX__) || defined(__MACH__)
   struct timespec tv;
   if (clock_gettime(CLOCK_MONOTONIC, &tv) == 0)
      time_ticks = (retro_perf_tick_t)tv.tv_sec * 1000000000 +
         (retro_perf_tick_t)tv.tv_nsec;

#elif defined(__GNUC__) && !defined(RARCH_CONSOLE)

#if defined(__i386__) || defined(__i486__) || defined(__i686__)
   __asm__ volatile ("rdtsc" : "=A" (time_ticks));
#elif defined(__x86_64__)
   unsigned a, d;
   __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
   time_ticks = (retro_perf_tick_t)a | ((retro_perf_tick_t)d << 32);
#endif

#elif defined(__ARM_ARCH_6__)
   __asm__ volatile( "mrc p15, 0, %0, c9, c13, 0" : "=r"(time_ticks) );
#elif defined(__CELLOS_LV2__) || defined(GEKKO) || defined(_XBOX360) || defined(__powerpc__) || defined(__ppc__) || defined(__POWERPC__)
   time_ticks = __mftb();
#elif defined(PSP) || defined(VITA)
   sceRtcGetCurrentTick(&time_ticks);
#elif defined(_3DS)
   time_ticks = svcGetSystemTick();
#elif defined(__mips__)
   struct timeval tv;
   gettimeofday(&tv,NULL);
   time_ticks = (1000000 * tv.tv_sec + tv.tv_usec);
#elif defined(_WIN32)
   long tv_sec, tv_usec;
   static const unsigned __int64 epoch = 11644473600000000Ui64;
   FILETIME file_time;
   SYSTEMTIME system_time;
   ULARGE_INTEGER ularge;

   GetSystemTime(&system_time);
   SystemTimeToFileTime(&system_time, &file_time);
   ularge.LowPart = file_time.dwLowDateTime;
   ularge.HighPart = file_time.dwHighDateTime;

   tv_sec = (long)((ularge.QuadPart - epoch) / 10000000L);
   tv_usec = (long)(system_time.wMilliseconds * 1000);

   time_ticks = (1000000 * tv_sec + tv_usec);
#endif

   return time_ticks / 1000000;
}

uint32_t LR_GetTicks(void)
{
   return (uint32_t)rarch_get_perf_counter();
}

void LR_FillRect(LR_Surface *surface, const void *rect_data, uint32_t color)
{
   unsigned i, j;
   unsigned short *pal = &d_8to16table[0];

   for(i = 0, j = 0; i < 256; i++, j += 3)
      *pal++ = color;

   for (i = 0; i < surface->surf->w; i++)
      for (j = 0; j < surface->surf->h; j++)
      {
         uint8_t *pix = (uint8_t*)surface->surf->pixels + j * surface->surf->pitch + i * 4;
         *(uint32_t*)pix = color;
      }
}

void LR_Delay(uint32_t ms)
{
   rarch_sleep(ms);
}

void LR_SetPalette(SDL_Surface *surface, int flags, LR_Color *colors, int firstcolor, int ncolors)
{
   unsigned i, j;
   unsigned short *pal = &d_8to16table[0];

   for(i = 0, j = 0; i < 256; i++, j += 3)
      *pal++ = MAKECOLOR(colors->r, colors->g, colors->b);

   SDL_SetPalette(surface, flags, (SDL_Color*)colors, firstcolor, ncolors);
}

int LR_SetColors(SDL_Surface *surface, LR_Color *colors, int firstcolor, int ncolors)
{
   return SDL_SetColors(surface, (SDL_Color*)colors, firstcolor, ncolors);
}

int LR_Init(uint32_t flags)
{
#ifdef __LIBRETRO__
   return 0;
#else
   return SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
#endif
}

void LR_Quit(void)
{
#ifdef __LIBRETRO__
   SDL_Quit();
#endif
}

SDL_Surface* LR_CreateRGBSurface(uint32_t flags,
      int    width,
      int    height,
      int    depth,
      uint32_t Rmask,
      uint32_t Gmask,
      uint32_t Bmask,
      uint32_t Amask)
{
   return LRSDL_CreateRGBSurface(flags, width, height, depth, Rmask, Gmask, Bmask, Amask);
}

void LR_FreeSurface(SDL_Surface* surface)
{
   LRSDL_FreeSurface(surface);
}

int LR_BlitSurface(LR_Surface *lr_src, SDL_Rect *srcrect, LR_Surface *lr_dst, SDL_Rect *dstrect)
{
#if 1
   //printf("src pitch %d, dst pitch %d\n", src->surf->pitch, dst->surf->pitch);
   SDL_Rect fulldst;
   int srcx, srcy, w, h;
   SDL_Surface *src = (SDL_Surface*)lr_src->surf;
   SDL_Surface *dst = (SDL_Surface*)lr_dst->surf;

   /* Make sure the surfaces aren't locked */
   if (!src || !dst) {
      LRSDL_SetError("SDL_UpperBlit: passed a NULL surface");
      return (-1);
   }
   if (src->locked || dst->locked) {
      LRSDL_SetError("Surfaces must not be locked during blit");
      return (-1);
   }

   /* If the destination rectangle is NULL, use the entire dest surface */
   if (dstrect == NULL) {
      fulldst.x = fulldst.y = 0;
      dstrect = &fulldst;
   }

   /* clip the source rectangle to the source surface */
   if (srcrect) {
      int maxw, maxh;

      srcx = srcrect->x;
      w = srcrect->w;
      if (srcx < 0) {
         w += srcx;
         dstrect->x -= srcx;
         srcx = 0;
      }
      maxw = src->w - srcx;
      if (maxw < w)
         w = maxw;

      srcy = srcrect->y;
      h = srcrect->h;
      if (srcy < 0) {
         h += srcy;
         dstrect->y -= srcy;
         srcy = 0;
      }
      maxh = src->h - srcy;
      if (maxh < h)
         h = maxh;

   } else {
      srcx = srcy = 0;
      w = src->w;
      h = src->h;
   }

   /* clip the destination rectangle against the clip rectangle */
   {
      SDL_Rect *clip = &dst->clip_rect;
      int dx, dy;

      dx = clip->x - dstrect->x;
      if (dx > 0) {
         w -= dx;
         dstrect->x += dx;
         srcx += dx;
      }
      dx = dstrect->x + w - clip->x - clip->w;
      if (dx > 0)
         w -= dx;

      dy = clip->y - dstrect->y;
      if (dy > 0) {
         h -= dy;
         dstrect->y += dy;
         srcy += dy;
      }
      dy = dstrect->y + h - clip->y - clip->h;
      if (dy > 0)
         h -= dy;
   }

   if (w > 0 && h > 0) {
      SDL_Rect sr;
      sr.x = srcx;
      sr.y = srcy;
      sr.w = dstrect->w = w;
      sr.h = dstrect->h = h;
      /*
       * rmask 0xF800
       * gmask 0x7E0
       * bmask 0x1F
       *
       * printf("src rmask: %d gmask: %d, bmask: %d\n", src->format->Rmask, src->format->Gmask, src->format->Bmask);
       * printf("dst rmask: %d gmask: %d, bmask: %d\n", dst->format->Rmask, dst->format->Gmask, dst->format->Bmask);
       */
      return LRSDL_LowerBlit(src, &sr, dst, dstrect);
   }
   dstrect->w = dstrect->h = 0;
#else
   unsigned i, j;
   if (srcrect)
      printf("src rect w %d h %d\n", srcrect->w, srcrect->h);
   if (dstrect)
      printf("dst rect w %d h %d\n", dstrect->w, dstrect->h);
   if (!src || !dst)
      return 0;
   for (i = 0; i < src->surf->w; i++)
   {
      for (j = 0; j < src->surf->h; j++)
      {
         SDL_Surface *src_sdl = src->surf;
         SDL_Surface *dst_sdl = dst->surf;
         uint16_t *src = (uint16_t*)src_sdl->pixels + j * src_sdl->pitch + i * 2;
         uint16_t *dst = (uint16_t*)dst_sdl->pixels + j * dst_sdl->pitch + i * 2;
         *(uint16_t*)dst = *src;
      }
   }
#endif
   return 0;
}


int LR_Flip(LR_Surface *screen)
{
#ifdef __LIBRETRO__
   /* video_cb call here */
   return 0;
#else
   return SDL_Flip(screen->surf);
#endif
}

SDL_Surface *LR_SetVideoMode(int width, int height, int bpp, uint32_t flags)
{
   return SDL_SetVideoMode(width, height, bpp, flags);
}

SDL_Surface *LR_ConvertSurface(LR_Surface *src, SDL_PixelFormat *fmt, uint32_t flags)
{
   return SDL_ConvertSurface(src->surf, fmt, flags);
}

uint32_t LR_MapRGB(SDL_PixelFormat *fmt, uint8_t r, uint8_t g, uint8_t b)
{
   return LRSDL_MapRGB(fmt, r, g, b);
}
