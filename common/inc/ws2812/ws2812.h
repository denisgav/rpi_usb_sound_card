#ifndef WS2812__H
#define WS2812__H

#include <stdio.h>
#include <stdlib.h>


typedef enum
{
  WS2812_ANIMATION_STREAMING = 25,
  WS2812_ANIMATION_NOT_MOUNTED = 250,
  WS2812_ANIMATION_MOUNTED = 1000,
  WS2812_ANIMATION_SUSPENDED = 2500,
} ws2812_animation_e;

int ws2812_init();

void ws2812_task(ws2812_animation_e animation);

#endif //WS2812__H